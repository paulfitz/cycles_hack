#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
using namespace std;

int main(int argc, char *argv[]) {
  if (argc!=3) {
    fprintf(stderr,"call as %s in.txt out.ppm\n", argv[0]);
    return 1;
  }

  FILE *fin = fopen(argv[1],"r");
  if (!fin) return 1;

  vector<int> vals;
  int w = 0;
  int h = 0;
  int mode = 0;
  char buf[2000];
  while (!feof(fin)) {
    fgets(&buf[0],sizeof(buf),fin);
    if (feof(fin)) break;
    char *token;
    char *at;
    token = strtok_r(buf," \n",&at);
    if (token) {
      if (strcmp(token,"999")==0) {
	break;
      }
      if (strcmp(token,"991")==0) {
	mode = 1;
      }
      if (strcmp(token,"993")==0) {
	mode = 3;
      }
      if (mode) {
	w = 0;
	while (token) {
	  token = strtok_r(NULL," \n",&at);
	  if (token) {
	    printf("[%s] ", token);
	    vals.push_back(atoi(token));
	    w++;
	  }
	}
	w /= mode;
	printf("\n");
	h++;
      }
    }
  }
  

  fclose(fin);


  FILE *fout = fopen(argv[2],"w");
  if (!fout) return 1;

  fprintf(fout, "P6\n%d %d\n%d\n", w, h, 255);
  int at = 0;
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      for (int m=0; m<mode; m++) {
	unsigned char ch = (unsigned char) vals[at];
	at++;	
	fwrite((void *) &ch, 1, (size_t) (1), fout);
	if (mode==1) {
	  fwrite((void *) &ch, 1, (size_t) (1), fout);
	  fwrite((void *) &ch, 1, (size_t) (1), fout);
	}
      }
    }
  }
  fclose(fout);

  return 0;
}

