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

#ifndef __UTIL_PROGRESS_H__
#define __UTIL_PROGRESS_H__

/* Progress
 *
 * Simple class to communicate progress status messages, timing information,
 * update notifications from a job running in another thread. All methods
 * except for the constructor/destructor are thread safe. */

#include "util_function.h"
#include "util_string.h"
#include "util_time.h"
#include "util_thread.h"

CCL_NAMESPACE_BEGIN

class Progress {
public:
	Progress()
	{
		sample = 0;
		start_time = time_dt();
		total_time = 0.0f;
		sample_time = 0.0f;
		status = "Initializing";
		substatus = "";
		update_cb = NULL;
		cancel = false;
		cancel_message = "";
		cancel_cb = NULL;
	}

	Progress(const Progress& progress)
	{
	  update_cb = progress.update_cb;
	  cancel_cb = progress.cancel_cb;
	  sample = progress.sample;
	  start_time = progress.start_time;
	  total_time = progress.total_time;
	  sample_time = progress.sample_time;
	  status = progress.status;
	  substatus = progress.substatus;
	  cancel = progress.cancel;
	  cancel_message = progress.cancel_message;
	}

	const Progress& operator=(const Progress& progress)
	{
	  update_cb = progress.update_cb;
	  cancel_cb = progress.cancel_cb;
	  sample = progress.sample;
	  start_time = progress.start_time;
	  total_time = progress.total_time;
	  sample_time = progress.sample_time;
	  status = progress.status;
	  substatus = progress.substatus;
	  cancel = progress.cancel;
	  cancel_message = progress.cancel_message;
	  // This is too, too strange
	  //progress.get_sample(sample, total_time, sample_time);
	  //	progress.get_status(status, substatus);
	  return *this;
	}

	/* cancel */
	void set_cancel(const string& cancel_message_)
	{
	  //thread_scoped_lock lock(progress_mutex);
		cancel_message = cancel_message_;
		cancel = true;
	}

	bool get_cancel()
	{
	  //printf("Checking for cancel...\n");
		if(!cancel && cancel_cb)
			cancel_cb();

		return cancel;
	}

	string get_cancel_message()
	{
	  //	thread_scoped_lock lock(progress_mutex);
		return cancel_message;
	}

	void set_cancel_callback(boost_function_void_void function)
	{
		cancel_cb = function;
	}

	/* sample and timing information */

	void set_start_time(double start_time_)
	{
	  //thread_scoped_lock lock(progress_mutex);

		start_time = start_time_;
	}

	void set_sample(int sample_, double sample_time_)
	{
	  //thread_scoped_lock lock(progress_mutex);

		sample = sample_;
		total_time = time_dt() - start_time;
		sample_time = sample_time_;
	}

	void get_sample(int& sample_, double& total_time_, double& sample_time_)
	{
	  //thread_scoped_lock lock(progress_mutex);

		sample_ = sample;
		total_time_ = (total_time > 0.0)? total_time: 0.0;
		sample_time_ = sample_time;
	}

	/* status messages */

	void set_status(const string& status_, const string& substatus_ = "")
	{
		{
		  //thread_scoped_lock lock(progress_mutex);
			status = status_;
			substatus = substatus_;
			total_time = time_dt() - start_time;
		}

		set_update();
	}

	void set_substatus(const string& substatus_)
	{
		{
		  //thread_scoped_lock lock(progress_mutex);
			substatus = substatus_;
			total_time = time_dt() - start_time;
		}

		set_update();
	}

	void get_status(string& status_, string& substatus_)
	{
	  //thread_scoped_lock lock(progress_mutex);
		status_ = status;
		substatus_ = substatus;
	}

	/* callback */

	void set_update()
	{
		if(update_cb)
			update_cb();
	}

	void set_update_callback(boost_function_void_void function)
	{
		update_cb = function;
	}

protected:
	boost_function_void_void update_cb;
	boost_function_void_void cancel_cb;

	int sample;

	double start_time;
	double total_time;
	double sample_time;

	string status;
	string substatus;

	bool cancel;
	string cancel_message;
};

CCL_NAMESPACE_END

#endif /* __UTIL_PROGRESS_H__ */

