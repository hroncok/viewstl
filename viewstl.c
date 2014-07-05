/*      ViewStl By Doug Dingus  Licensed Under GPL   */

/* fix crash when invoked from really long command line (long file path) */
/* this probably has to do with the display of the filename in the window title */
/* probably need to allocate more string space for long args (Mostly Fixed)*/

/* Basic Includes for OGL, GLUT, and other */
/* WIN32 needs some additional ones  */

/* Integrated patches from Hans  hhof@users.sourceforge.net 0.35 */

#include <GL/gl.h>   /* OpenGL itself. */
#include <GL/glu.h>  /* GLU support library */
#include <GL/glut.h> /* GLUT support library */

#include <unistd.h>     /* needed to sleep*/
#include <stdlib.h>    /* for malloc() and exit() */
#include <stdio.h>      /* needed for file & screen i/o */
#include <string.h>    /* for strcpy and relatives */
#include <time.h>       /* For our FPS */
#include <math.h>       /* Gotta do some trig */

#include <admesh/stl.h>   /* STL loading */

/* ASCII code for the various keys used */
#define ESCAPE 27     /* esc */
#define ROTyp  105    /* i   */
#define ROTym  109    /* m   */
#define ROTxp  107    /* k   */
#define ROTxm  106    /* j   */
#define SCAp   43     /* +   */
#define SCAm   45     /* -   */
/* The mouse buttons */
#define LMB 0	/* Left */
#define MMB 1   /* Middle */
#define RMB 2   /* Right */
/* Number of samples for frame rate */
#define FR_SAMPLES 10
/* ViewFlag arguments */
#define ORTHO 1
#define PERSPECTIVE 0
#define YES 1
#define NO 0
/* Other Constants */
#define PI 3.14159265358979323846
#define FOV 30   /* Field of view for perspective mode */
#define PCR 2  /* Helps pan adapt to varying model sizes */


/* Declarations ------------------------------------- */

char arg1[100], arg2[20], arg3[20];
FILE *filein; /* Filehandle for the STL file to be viewed */
int window; /* The number of our GLUT window */
int x;  /* general index */
float scale = 0;
float ROTx = 0, ROTy = 0;
float PANx = 0, PANy = 0;
int MOUSEx = 0, MOUSEy = 0, BUTTON = 0;
float extent_pos_x = 0, extent_pos_y = 0, extent_pos_z = 0;
float extent_neg_x = 0, extent_neg_y = 0, extent_neg_z = 0;
float rot_center_x, rot_center_y, rot_center_z;
/* Stuff for the frame rate calculation */
int FrameCount=0;
float FrameRate=0;
/* Settings for the light  */
float Light_Ambient[]=  { 0.0f, 0.0f, 0.0f, 1.0f };
float Light_Diffuse[]=  { 1.0f, 1.0f, 1.0f, 1.0f };
float Light_Position[]= { 2.0f, 2.0f, 0.0f, 1.0f };
int ViewFlag = 0; /* 0=perspective, 1=ortho */
float oScale = 1.0;
int update = YES, idle_draw = YES;
float Z_Depth = -5, Big_Extent = 10;
int verbose = NO;
stl_file* stl;
size_t v;


/* This function reads through the polygons to find the */
/* largest and smallest vertices in the model.  This data will be used by the */
/* transform_model function to center the part at the origin for rotation */
/* and easy autoscale */
static void FindExtents() {
  int x;
  /* Find geometry extents */
  for (x = 0 ; x < stl->stats.number_of_facets ; x = x + 1) {
    /* first the positive vertex check for each poly */
    /* this code needs to be way shorter, but go with what works */
    for (v = 0; v < 3; v++) {
      if (stl->facet_start[x].vertex[v].x > extent_pos_x)
        extent_pos_x = stl->facet_start[x].vertex[0].x;
      if (stl->facet_start[x].vertex[v].y > extent_pos_y)
        extent_pos_y = stl->facet_start[x].vertex[0].y;
      if (stl->facet_start[x].vertex[v].z > extent_pos_z)
        extent_pos_z = stl->facet_start[x].vertex[0].z;

      /* then the negative checks */
      if (stl->facet_start[x].vertex[v].x < extent_neg_x)
        extent_neg_x = stl->facet_start[x].vertex[0].x;
      if (stl->facet_start[x].vertex[v].y < extent_neg_y)
        extent_neg_y = stl->facet_start[x].vertex[0].y;
      if (stl->facet_start[x].vertex[v].z < extent_neg_z)
        extent_neg_z = stl->facet_start[x].vertex[0].z;
    }
  }
}


/* This translates the center of the rectangular bounding box surrounding */
/* the model to the origin.  Makes for good rotation.  Also it does a quick */
/* Z depth calculation that will be used bring the model into view (mostly) */

static void TransformToOrigin() {
  int x;
  float LongerSide, ViewAngle;

  /* first transform into positive quadrant */
  for (x = 0 ; x < stl->stats.number_of_facets ; x = x + 1) {
    for (v = 0; v < 3; v++) {
      stl->facet_start[x].vertex[v].x = stl->facet_start[x].vertex[v].x + (0 - extent_neg_x);
      stl->facet_start[x].vertex[v].y = stl->facet_start[x].vertex[v].y + (0 - extent_neg_y);
      stl->facet_start[x].vertex[v].z = stl->facet_start[x].vertex[v].z + (0 - extent_neg_z);
    }
  }
  FindExtents();
  /* Do quick Z_Depth calculation while part resides in ++ quadrant */
  /* Convert Field of view to radians */
  ViewAngle = ((FOV / 2) * (PI / 180));
  if (extent_pos_x > extent_pos_y)
    LongerSide = extent_pos_x;
  else
    LongerSide = extent_pos_y;

  /* Put the result where the main drawing function can see it */
  Z_Depth = ((LongerSide / 2) / tanf(ViewAngle));
  Z_Depth = Z_Depth * -1;

  /* Do another calculation for clip planes */
  /* Take biggest part dimension and use it to size the planes */
  if ((extent_pos_x > extent_pos_y) && (extent_pos_x > extent_pos_z))
    Big_Extent = extent_pos_x;
  if ((extent_pos_y > extent_pos_x) && (extent_pos_y > extent_pos_z))
    Big_Extent = extent_pos_y;
  if ((extent_pos_z > extent_pos_y) && (extent_pos_z > extent_pos_x))
    Big_Extent = extent_pos_z;

  /* Then calculate center and put it back to origin */
  for (x = 0 ; x < stl->stats.number_of_facets ; x = x + 1) {
    for (v = 0; v < 3; v++) {
      stl->facet_start[x].vertex[v].x = stl->facet_start[x].vertex[v].x - (extent_pos_x/2);
      stl->facet_start[x].vertex[v].y = stl->facet_start[x].vertex[v].y - (extent_pos_y/2);
      stl->facet_start[x].vertex[v].z = stl->facet_start[x].vertex[v].z - (extent_pos_z/2);
    }
  }

}


/* Sets up Projection matrix according to command switch -o or -p */
/* called from initgl and the window resize function */
static void SetView(int Width, int Height) {
  float aspect = (float)Width / (float)Height;
  if (verbose)
    printf("Window Aspect is: %f\n", aspect);


  if (ViewFlag == PERSPECTIVE) {
    /* Calculate The Aspect Ratio Of The Window*/
    gluPerspective(FOV,(GLfloat)Width/(GLfloat)Height,0.1f,(Z_Depth + Big_Extent));
  }

  if (ViewFlag == ORTHO) {
    /* glOrtho(left, right, bottom, top, near, far) */
    glOrtho((extent_neg_x*1.2f), (extent_pos_x*1.2f), (extent_neg_y*aspect), (extent_pos_y*aspect), -1.0f, 10.0f);
  }

}

/* A general OpenGL initialization function. */
/* Called once from main() */
void InitGL(int Width, int Height) {        /* We call this right after our OpenGL window is created.*/
  glClearColor(0.15f, 0.15f, 0.2f, 0.0f);		/* This Will Clear The Background Color To Dark Red*/
  glClearDepth(1.0);				/* Enables Clearing Of The Depth Buffer*/
  glDepthFunc(GL_LESS);			        /* The Type Of Depth Test To Do*/
  glEnable(GL_DEPTH_TEST);		        /* Enables Depth Testing*/
  glShadeModel(GL_SMOOTH);			/* Enables Smooth Color Shading*/
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();				/* Reset The Projection Matrix*/

  SetView(Width, Height);  /* Setup the View Matrix */

  glMatrixMode(GL_MODELVIEW);
  glEnable(GL_LIGHTING);
  /* Set up some lights and turn them on. */
  glLightfv(GL_LIGHT1, GL_POSITION, Light_Position);
  glLightfv(GL_LIGHT1, GL_AMBIENT,  Light_Ambient);
  glLightfv(GL_LIGHT1, GL_DIFFUSE,  Light_Diffuse);
  glEnable (GL_LIGHT1);
}







/* The function called when our window is resized  */
void ReSizeGLScene(int Width, int Height) {
  if (Height==0)	/* Prevent A Divide By Zero If The Window Is Too Small*/
    Height=1;

  glViewport(0, 0, Width, Height);    /* Reset The Current Viewport And Perspective Transformation*/
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  SetView(Width, Height);

  glMatrixMode(GL_MODELVIEW);
  update = YES;
}

/* The main drawing function. */
void DrawGLScene()

{
  if ((!update) && (!idle_draw))
    return;
  update = NO;
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);	/* Clear The Screen And The Depth Buffer*/
  /* for now use two different methods of scaling depending on ortho or perspective */
  if (ViewFlag == PERSPECTIVE) {
    glLoadIdentity();
    glTranslatef(PANx, PANy, (Z_Depth + scale));
    glRotatef(ROTx, 1.0f, 0.0f, 0.0f);
    glRotatef(ROTy, 0.0f, 1.0f, 0.0f);
  }

  /* ortho scaling is broken */
  if (ViewFlag == ORTHO) {
    glLoadIdentity();
    glTranslatef(PANx, PANy, -5.0);
    glRotatef(ROTx, 1.0f, 0.0f, 0.0f);
    glRotatef(ROTy, 0.0f, 1.0f, 0.0f);

  }
  for(x = 0 ; x < stl->stats.number_of_facets ; x++) {
    glBegin(GL_POLYGON);
    glNormal3f(stl->facet_start[x].normal.x, stl->facet_start[x].normal.y, stl->facet_start[x].normal.z);
    glVertex3f(stl->facet_start[x].vertex[0].x, stl->facet_start[x].vertex[0].y, stl->facet_start[x].vertex[0].z);
    glVertex3f(stl->facet_start[x].vertex[1].x, stl->facet_start[x].vertex[1].y, stl->facet_start[x].vertex[1].z);
    glVertex3f(stl->facet_start[x].vertex[2].x, stl->facet_start[x].vertex[2].y, stl->facet_start[x].vertex[2].z);
    glEnd();
  }
  /* swap the buffers to display, since double buffering is used.*/
  glutSwapBuffers();

  /* cut down on the number of redraws on window title.  Only draw once per sample*/
  if (FrameCount == 0) {
    glutSetWindowTitle(arg1);
  }
}






/* The function called whenever a mouse button event occurs */
void mouseButtonPress(int button, int state, int x, int y) {
  int oriantation;
  if (verbose)
    printf(" mouse--> %i %i %i %i\n", button, state, x, y);

  if ((button == 3) || (button == 4)) {
    if (state == GLUT_UP) return;
    oriantation = button*2 - 7;
    scale = scale + oriantation*(10*(Z_Depth+scale))*.01;
    oScale = oScale + oriantation*(10*(Z_Depth+scale))*.01;
  } else {
    BUTTON = button;
    MOUSEx = x;
    MOUSEy = y;
  }
  update = YES;
}






/* The function called whenever a mouse motion event occurs */
void mouseMotionPress(int x, int y) {
  if (verbose)
    printf("You did this with the mouse--> %i %i\n", x, y);
  if (BUTTON == RMB) {
    PANx = PANx + ((MOUSEx - x)*(tanf(0.26179939)*(Z_Depth+scale)))*.005;
    PANy = PANy - ((MOUSEy - y)*(tanf(0.26179939)*(Z_Depth+scale)))*.005;
    MOUSEx = x;
    MOUSEy = y;
  }
  if (BUTTON == LMB) {
    ROTy = ROTy - ((MOUSEx - x)*0.5);
    ROTx = ROTx - ((MOUSEy - y)*0.5);
    MOUSEx = x;
    MOUSEy = y;
  }
  if (BUTTON == MMB) {
    scale = scale + ((MOUSEy - y)*(tanf(0.26179939)*(Z_Depth+scale)))*.01;
    oScale = oScale + ((MOUSEy - y)*(tanf(0.26179939)*(Z_Depth+scale)))*.01;
    /* scale = scale - ((MOUSEy - y)*0.05);
    oScale = oScale - ((MOUSEy - y)*0.05);  */
    MOUSEx = x;
    MOUSEy = y;
  }
  update = YES;
}





/* The function called whenever a key is pressed. */
void keyPressed(unsigned char key, int x, int y) {
  /* Keyboard debounce */
  /* I don't know which lib has this in win32 */
  sleep(0.1);

  /* Pressing escape kills everything --Have a nice day! */
  if (key == ESCAPE) {
    /* shut down our window */
    glutDestroyWindow(window);
    stl_close(stl);
    free(stl);
    /* exit the program...normal termination. */
    exit(0);
  }
  if (verbose)
    printf("You pressed key--> %i at %i, %i screen location\n", key, x, y);
  update = YES;
}




/* This function is for the special keys.  */
/* The dynamic viewing keys need to be time based */
void specialkeyPressed (int key, int x, int y) {
  /* keep track of time between calls, if it exceeds a certian value, then */
  /* assume the user has released the key and wants to start fresh */
  static int first = YES;
  static clock_t last=0;
  clock_t now;
  float delta;

  /* Properly initialize the MOUSE vars on first time through this function */
  if (first) {
    first = NO;
    MOUSEx = x;
    MOUSEy = y;
  }

  /* If the clock exceeds a reasonable value, assume user has released F key */      now  = clock();
  delta= (now - last) / (float) CLOCKS_PER_SEC;
  last = now;
  if (delta > 0.1) {
    MOUSEx = x;
    MOUSEy = y;

  }

  /* Pan is assigned the F1 key */
  if (key == 1) {
    PANx = PANx + ((MOUSEx - x)*(tanf(0.26179939)*(Z_Depth+scale)))*.005;
    PANy = PANy - ((MOUSEy - y)*(tanf(0.26179939)*(Z_Depth+scale)))*.005;
    MOUSEx = x;
    MOUSEy = y;
  }

  /* Zoom or Scale is the F2 key */
  if (key == 2) {
    scale = scale + ((MOUSEy - y)*(tanf(0.26179939)*(Z_Depth+scale)))*.01;
    oScale = oScale + ((MOUSEy - y)*(tanf(0.26179939)*(Z_Depth+scale)))*.01;
    /* scale = scale - ((MOUSEy - y)*0.05);
    oScale = oScale - ((MOUSEy - y)*0.05);  */
    MOUSEx = x;
    MOUSEy = y;
  }

  /* Rotate assigned the F3 key */

  if (key == 3) {
    ROTy = ROTy - ((MOUSEx - x)*0.5);
    ROTx = ROTx - ((MOUSEy - y)*0.5);
    MOUSEx = x;
    MOUSEy = y;
  }

  /* Cool Display Stuff... */
  if (key == 4) {
    glPolygonMode(GL_FRONT, GL_FILL);
  }
  if (key == 5) {
    glPolygonMode(GL_FRONT, GL_LINE);
    glPolygonMode(GL_BACK, GL_FILL);
  }
  if (key == 6) {
    glPolygonMode(GL_FRONT, GL_LINE);
    glPolygonMode(GL_BACK, GL_POINT);
  }
  if (key == 7) {
    glDisable(GL_CULL_FACE);
  }
  if (key == 8) {
    glEnable(GL_CULL_FACE);
  }
  if (verbose)
    printf("Special Key--> %i at %i, %i screen location\n", key, x, y);
  update = YES;
}







int main(int argc, char *argv[]) {

  /* Begin parsing command args.  Lame, but it works :)  */
  printf("  viewstl: ");
  filein = fopen(argv[1], "r");
  if (filein == 0) {
    printf("This is how you invoke the viewer... \n");
    printf("           Usage:  viewstl [file] (-o or -p) (-f or -v)\n");
    printf("           Valid Options are: -o (Ortho View EXPEREMENTAL)\n");
    printf("                              -p (Perspective View)\n");
    printf("                              -f (Redraw only on view change)\n");
    printf("                              -v (Report debug info to STDOUT)\n");
    exit(1);
  }

  fclose(filein);

  if (argc > 2) {
    strcpy(arg1, "-o");
    if (strcmp(argv[2], arg1) == 0) {
      printf("Running in Ortho View\n");
      ViewFlag = ORTHO;
    }
  }
  if (ViewFlag == PERSPECTIVE) {
    printf("Running in Perspective View\n");
  }
  if (argc == 4) {
    strcpy(arg1, "-f");
    if (strcmp(argv[3], arg1) == 0) {
      printf("           Redrawing only on view change\n");
      idle_draw = NO;
    }
    strcpy(arg1, "-v");
    if (strcmp(argv[3], arg1) == 0) {
      printf("           Debug Output Enabled\n");
      verbose = YES;
    }


  }

  stl = (stl_file*)malloc(sizeof(stl_file));
  memset(stl,0,sizeof(stl_file));
  stl_open(stl,argv[1]);

  /* repair the normals a bit, to see the STL fine */
  stl_fix_normal_values(stl);

  FindExtents();

  /* Print the result of the extent calc */
  if (verbose) {
    printf("           Part extents are: x, y, z\n");
    printf("           %f, %f, %f\n", extent_pos_x, extent_pos_y, extent_pos_z);
    printf("           %f, %f, %f\n", extent_neg_x, extent_neg_y, extent_neg_z);
  }

  TransformToOrigin();

  if (verbose)
    printf("           File Processed\n");

  /* Initialize GLUT state - glut will take any command line arguments that pertain to it or
     X Windows - look at its documentation at http://reality.sgi.com/mjk/spec3/spec3.html */
  glutInit(&argc, argv);

  /* Select type of Display mode:
       Double buffer
       RGBA color
       Depth buffered for automatic clipping */
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

  /* get a 640 x 480 window */
  glutInitWindowSize(640, 480);

  strcpy (arg1, argv[1]);
  strcat (arg1, " - viewstl");

  window = glutCreateWindow(arg1);

  /* Register the event callback functions since we are using GLUT */
  glutDisplayFunc(&DrawGLScene); /* Register the function to do all our OpenGL drawing. */
  glutIdleFunc(&DrawGLScene); /* Even if there are no events, redraw our gl scene. */
  glutReshapeFunc(&ReSizeGLScene); /* Register the function called when our window is resized. */
  glutKeyboardFunc(&keyPressed); /* Register the function called when the keyboard is pressed. */
  glutSpecialFunc(&specialkeyPressed); /* Register the special key function */
  glutMouseFunc(&mouseButtonPress); /* Register the function called when the mouse buttons are pressed */
  glutMotionFunc(&mouseMotionPress); /*Register the mouse motion function */

  /* Initialize our window. */
  InitGL(640, 480);

  /* Start Event Processing Engine */
  glutMainLoop();

  return 1;
}


