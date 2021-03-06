/*
 * Copyright 2011, Blender Foundation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "graph.h"
#include "light.h"
#include "mesh.h"
#include "object.h"
#include "scene.h"
#include "nodes.h"
#include "shader.h"

#include "blender_sync.h"
#include "blender_util.h"

#include "util_foreach.h"

CCL_NAMESPACE_BEGIN

/* Utilities */

bool BlenderSync::object_is_modified(BL::Object b_ob)
{
	/* test if we can instance or if the object is modified */
	if(ccl::object_is_modified(b_ob, b_scene, preview)) {
		/* modifiers */
		return true;
	}
	else {
		/* object level material links */
		BL::Object::material_slots_iterator slot;
		for(b_ob.material_slots.begin(slot); slot != b_ob.material_slots.end(); ++slot)
			if(slot->link() == BL::MaterialSlot::link_OBJECT)
				return true;
	}

	return false;
}

bool BlenderSync::object_is_mesh(BL::Object b_ob)
{
	BL::ID b_ob_data = b_ob.data();

	return (b_ob_data && (b_ob_data.is_a(&RNA_Mesh) ||
		b_ob_data.is_a(&RNA_Curve) || b_ob_data.is_a(&RNA_MetaBall)));
}

bool BlenderSync::object_is_light(BL::Object b_ob)
{
	BL::ID b_ob_data = b_ob.data();

	return (b_ob_data && b_ob_data.is_a(&RNA_Lamp));
}

static uint object_ray_visibility(BL::Object b_ob)
{
	PointerRNA cvisibility = RNA_pointer_get(&b_ob.ptr, "cycles_visibility");
	uint flag = 0;

	flag |= get_boolean(cvisibility, "camera")? PATH_RAY_CAMERA: 0;
	flag |= get_boolean(cvisibility, "diffuse")? PATH_RAY_DIFFUSE: 0;
	flag |= get_boolean(cvisibility, "glossy")? PATH_RAY_GLOSSY: 0;
	flag |= get_boolean(cvisibility, "transmission")? PATH_RAY_TRANSMIT: 0;
	flag |= get_boolean(cvisibility, "shadow")? PATH_RAY_SHADOW: 0;

	return flag;
}

/* Light */

void BlenderSync::sync_light(BL::Object b_parent, int b_index, BL::Object b_ob, Transform& tfm)
{
	/* test if we need to sync */
	Light *light;
	ObjectKey key(b_parent, b_index, b_ob);

	if(!light_map.sync(&light, b_ob, b_parent, key))
		return;
	
	BL::Lamp b_lamp(b_ob.data());

	/* type */
	switch(b_lamp.type()) {
		case BL::Lamp::type_POINT: {
			BL::PointLamp b_point_lamp(b_lamp);
			light->size = b_point_lamp.shadow_soft_size();
			light->type = LIGHT_POINT;
			break;
		}
		case BL::Lamp::type_SPOT: {
			BL::SpotLamp b_spot_lamp(b_lamp);
			light->size = b_spot_lamp.shadow_soft_size();
			light->type = LIGHT_POINT;
			break;
		}
		case BL::Lamp::type_HEMI: {
			light->type = LIGHT_DISTANT;
			light->size = 0.0f;
			break;
		}
		case BL::Lamp::type_SUN: {
			BL::SunLamp b_sun_lamp(b_lamp);
			light->size = b_sun_lamp.shadow_soft_size();
			light->type = LIGHT_DISTANT;
			break;
		}
		case BL::Lamp::type_AREA: {
			BL::AreaLamp b_area_lamp(b_lamp);
			light->size = 1.0f;
			light->axisu = make_float3(tfm.x.x, tfm.y.x, tfm.z.x);
			light->axisv = make_float3(tfm.x.y, tfm.y.y, tfm.z.y);
			light->sizeu = b_area_lamp.size();
			if(b_area_lamp.shape() == BL::AreaLamp::shape_RECTANGLE)
				light->sizev = b_area_lamp.size_y();
			else
				light->sizev = light->sizeu;
			light->type = LIGHT_AREA;
			break;
		}
	}

	/* location and (inverted!) direction */
	light->co = make_float3(tfm.x.w, tfm.y.w, tfm.z.w);
	light->dir = -make_float3(tfm.x.z, tfm.y.z, tfm.z.z);

	/* shader */
	vector<uint> used_shaders;

	find_shader(b_lamp, used_shaders, scene->default_light);

	if(used_shaders.size() == 0)
		used_shaders.push_back(scene->default_light);

	light->shader = used_shaders[0];

	/* shadow */
	PointerRNA clamp = RNA_pointer_get(&b_lamp.ptr, "cycles");
	light->cast_shadow = get_boolean(clamp, "cast_shadow");

	/* tag */
	light->tag_update(scene);
}

void BlenderSync::sync_background_light()
{
	BL::World b_world = b_scene.world();

	if(b_world) {
		PointerRNA cworld = RNA_pointer_get(&b_world.ptr, "cycles");
		bool sample_as_light = get_boolean(cworld, "sample_as_light");

		if(sample_as_light) {
			/* test if we need to sync */
			Light *light;
			ObjectKey key(b_world, 0, b_world);

			if(light_map.sync(&light, b_world, b_world, key) ||
			   world_recalc ||
			   b_world.ptr.data != world_map)
			{
				light->type = LIGHT_BACKGROUND;
				light->map_resolution  = get_int(cworld, "sample_map_resolution");
				light->shader = scene->default_background;

				light->tag_update(scene);
				light_map.set_recalc(b_world);
			}
		}
	}

	world_map = b_world.ptr.data;
	world_recalc = false;
}

/* Object */

void BlenderSync::sync_object(BL::Object b_parent, int b_index, BL::Object b_ob, Transform& tfm, uint layer_flag)
{
	/* light is handled separately */
	if(object_is_light(b_ob)) {
		sync_light(b_parent, b_index, b_ob, tfm);
		return;
	}

	/* only interested in object that we can create meshes from */
	if(!object_is_mesh(b_ob))
		return;

	/* test if we need to sync */
	ObjectKey key(b_parent, b_index, b_ob);
	Object *object;
	bool object_updated = false;

	if(object_map.sync(&object, b_ob, b_parent, key))
		object_updated = true;
	
	/* holdout? */
	bool holdout = (layer_flag & render_layer.holdout_layer) != 0;

	/* mesh sync */
	object->mesh = sync_mesh(b_ob, holdout, object_updated);

	/* object sync */
	if(object_updated || (object->mesh && object->mesh->need_update)) {
		object->name = b_ob.name().c_str();
		object->pass_id = b_ob.pass_index();
		object->tfm = tfm;

		/* visibility flags for both parent */
		object->visibility = object_ray_visibility(b_ob) & PATH_RAY_ALL;
		if(b_parent.ptr.data != b_ob.ptr.data)
			object->visibility &= object_ray_visibility(b_parent);

		/* camera flag is not actually used, instead is tested
		   against render layer flags */
		if(object->visibility & PATH_RAY_CAMERA) {
			object->visibility |= layer_flag << PATH_RAY_LAYER_SHIFT;
			object->visibility &= ~PATH_RAY_CAMERA;
		}

		object->tag_update(scene);
	}
}

/* Object Loop */

void BlenderSync::sync_objects(BL::SpaceView3D b_v3d)
{
	/* layer data */
	uint scene_layer = render_layer.scene_layer;
	
	/* prepare for sync */
	light_map.pre_sync();
	mesh_map.pre_sync();
	object_map.pre_sync();
	mesh_synced.clear();

	/* object loop */
	BL::Scene::objects_iterator b_ob;

	for(b_scene.objects.begin(b_ob); b_ob != b_scene.objects.end(); ++b_ob) {
		bool hide = (b_v3d)? b_ob->hide(): b_ob->hide_render();
		uint ob_layer = get_layer(b_ob->layers());

		if(!hide && (ob_layer & scene_layer)) {
			if(b_ob->is_duplicator()) {
				/* dupli objects */
				object_create_duplilist(*b_ob, b_scene);

				BL::Object::dupli_list_iterator b_dup;
				int b_index = 0;

				for(b_ob->dupli_list.begin(b_dup); b_dup != b_ob->dupli_list.end(); ++b_dup) {
					Transform tfm = get_transform(b_dup->matrix());
					BL::Object b_dup_ob = b_dup->object();
					bool dup_hide = (b_v3d)? b_dup_ob.hide(): b_dup_ob.hide_render();

					if(!(b_dup->hide() || dup_hide))
						sync_object(*b_ob, b_index, b_dup_ob, tfm, ob_layer);

					b_index++;
				}

				object_free_duplilist(*b_ob);

				hide = true;
			}

			/* check if we should render or hide particle emitter */
			BL::Object::particle_systems_iterator b_psys;
			bool render_emitter = false;

			for(b_ob->particle_systems.begin(b_psys); b_psys != b_ob->particle_systems.end(); ++b_psys) {
				if(b_psys->settings().use_render_emitter()) {
					hide = false;
					render_emitter = true;
				}
				else if(!render_emitter)
					hide = true;
			}

			if(!hide) {
				/* object itself */
				Transform tfm = get_transform(b_ob->matrix_world());
				sync_object(*b_ob, 0, *b_ob, tfm, ob_layer);
			}
		}
	}

	sync_background_light();

	/* handle removed data and modified pointers */
	if(light_map.post_sync())
		scene->light_manager->tag_update(scene);
	if(mesh_map.post_sync())
		scene->mesh_manager->tag_update(scene);
	if(object_map.post_sync())
		scene->object_manager->tag_update(scene);
	mesh_synced.clear();
}

CCL_NAMESPACE_END

