// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <iterator>
#include <math.h>
#include <vector>

// specify that we want the OpenGL core profile before including GLFW headers
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

//Globals
float picWidth;
float picHeight;
int screenSize = 512;
float orien = 0;
float mag = 1;
std::string picName = "";
int colourEffect;
int blur;
int edgeEffect;
float centerX;
float centerY;
float pictureCenterX;
float pictureCenterY;
float oldPictureCenterX;
float oldPictureCenterY;
bool pressed;
int rotateFlag = 0;

using namespace std;
// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

void PicGen(std::string name);

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

struct MyShader
{
	// OpenGL names for vertex and fragment shaders, shader program
	GLuint  vertex;
	GLuint  fragment;
	GLuint  program;

	// initialize shader and program names to zero (OpenGL reserved value)
	MyShader() : vertex(0), fragment(0), program(0)
	{}
};

//global
MyShader shader;

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader)
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource("fragment.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	// link shader program
	shader->program = LinkProgram(shader->vertex, shader->fragment);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	glDeleteProgram(shader->program);
	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing textures

struct MyTexture
{
	GLuint textureID;
	GLuint target;
        int width;
        int height;

	// initialize object names to zero (OpenGL reserved value)
	MyTexture() : textureID(0), target(0), width(0), height(0)
	{}
};

bool InitializeTexture(MyTexture* texture, const char* filename, GLuint target = GL_TEXTURE_2D)
{
	int numComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename, &texture->width, &texture->height, &numComponents, 0);
	if (data != nullptr)
	{
		texture->target = target;
		glGenTextures(1, &texture->textureID);
		glBindTexture(texture->target, texture->textureID);
		GLuint format = numComponents == 3 ? GL_RGB : GL_RGBA;
                //cout << numComponents << endl;
		glTexImage2D(texture->target, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);

		// Note: Only wrapping modes supported for GL_TEXTURE_RECTANGLE when defining
		// GL_TEXTURE_WRAP are GL_CLAMP_TO_EDGE or GL_CLAMP_TO_BORDER
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                picWidth = texture->width;
                picHeight = texture->height;
                //cout << picWidth << endl;

		// Clean up
		glBindTexture(texture->target, 0);
		stbi_image_free(data);
		return !CheckGLErrors();
	}
	return true; //error
}

// deallocate texture-related objects
void DestroyTexture(MyTexture *texture)
{
	glBindTexture(texture->target, 0);
	glDeleteTextures(1, &texture->textureID);
}

void SaveImage(const char* filename, int width, int height, unsigned char *data, int numComponents = 3, int stride = 0)
{
	if (!stbi_write_png(filename, width, height, numComponents, data, stride))
		cout << "Unable to save image: " << filename << endl;
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry)
{
	//cout << mag << endl;
	float heightRatio = 1;
	float widthRatio = 1;
	
	//if((picHeight>screenSize)||(picWidth>screenSize))
	//{
		if(picWidth>picHeight)
		{
			heightRatio = 1;
			widthRatio = picWidth/picHeight;
		}
		else if(picHeight>picWidth)
		{
			heightRatio = picHeight/picWidth;
			widthRatio = 1;
		}
	//}

         int tempPictureCenterX = pictureCenterX;
         int tempPictureCenterY = pictureCenterY;
         pictureCenterX = pictureCenterX - oldPictureCenterX;
         pictureCenterY = pictureCenterY - oldPictureCenterY;
         oldPictureCenterX = tempPictureCenterX;
         oldPictureCenterY = tempPictureCenterY;

         //four vertex positions and assocated colours of a polygon
        const GLfloat vertices[][2] = {
                { (((-1.f/heightRatio)*cos(orien)-(-1.f/widthRatio)*sin(orien))*mag)+pictureCenterX, (((-1.f/heightRatio)*sin(orien)+(-1.f/widthRatio)*cos(orien))*mag)+pictureCenterY },
                { (((-1.f/heightRatio)*cos(orien)-(1.f/widthRatio)*sin(orien))*mag)+pictureCenterX, (((-1.f/heightRatio)*sin(orien)+(1.f/widthRatio)*cos(orien))*mag)+pictureCenterY},
                { (((1.f/heightRatio)*cos(orien)-(1.f/widthRatio)*sin(orien))*mag)+pictureCenterX, (((1.f/heightRatio)*sin(orien)+(1.f/widthRatio)*cos(orien))*mag)+pictureCenterY },

                { (((1.f/heightRatio)*cos(orien)-(1.f/widthRatio)*sin(orien))*mag)+pictureCenterX, (((1.f/heightRatio)*sin(orien)+(1.f/widthRatio)*cos(orien))*mag)+pictureCenterY },
                { (((1.f/heightRatio)*cos(orien)-(-1.f/widthRatio)*sin(orien))*mag)+pictureCenterX, (((1.f/heightRatio)*sin(orien)+(-1.f/widthRatio)*cos(orien))*mag)+pictureCenterY },
                { (((-1.f/heightRatio)*cos(orien)-(-1.f/widthRatio)*sin(orien))*mag)+pictureCenterX, (((-1.f/heightRatio)*sin(orien)+(-1.f/widthRatio)*cos(orien))*mag)+pictureCenterY },
        };

        const GLfloat colours[][3] = {
				{ 1.0f, 0.0f, 0.0f },
				{ 0.0f, 1.0f, 0.0f },
				{ 0.0f, 0.0f, 1.0f },
                { 0.0f, 0.0f, 1.0f },
                { 0.0f, 0.0f, 1.0f },
                { 1.0f, 0.0f, 0.0f }
	};
        const GLfloat textureCoordinates[][2] = {
				{0.f,0.f},
				{0.f,picHeight},
				{picWidth,picHeight},

				{picWidth,picHeight},
				{picWidth,0.f},
				{0.f,0.f}
        };

        //cout << picWidth << endl;

        geometry->elementCount = 6;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;
        const GLuint TEXTURE_INDEX = 2;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        //Create array buffer for storing texture coordinates
        glGenBuffers(1,&geometry->textureBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordinates), textureCoordinates, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

        //TEll OpenGL how our texture VBO is formatted and enable it
        glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
        glVertexAttribPointer(TEXTURE_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(TEXTURE_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry(MyGeometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
        glDeleteBuffers(1, &geometry->textureBuffer);
	glDeleteBuffers(1, &geometry->vertexBuffer);
        glDeleteBuffers(1, &geometry->colourBuffer);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene(MyGeometry *geometry, MyTexture* texture, MyShader *shader)
{
        //cout << "Rednering Scene" << endl;
        // clear screen to a dark grey colour
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->program);
        glBindVertexArray(geometry->vertexArray);
        glBindTexture(texture->target, texture->textureID);
        glDrawArrays(GL_TRIANGLES, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
        glBindTexture(texture->target, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
        glUseProgram(shader.program);
        MyGeometry geometry;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

        //When 1 is pressed display image 1
        else if(key == GLFW_KEY_1 && action == GLFW_PRESS)
        {
			blur = 0;
			GLint locB = glGetUniformLocation(shader.program,"blur");
            if (locB != -1)
            {
              glUniform1i(locB, blur);
            }
            oldPictureCenterX = pictureCenterX = 0;
            oldPictureCenterY = pictureCenterY = 0;
            colourEffect = 0;
            GLint locC = glGetUniformLocation(shader.program,"colourEffect");
            if (locC != -1)
            {
              glUniform1i(locC, colourEffect);
            }
            edgeEffect = 0;
            GLint locE = glGetUniformLocation(shader.program,"edgeEffect");
            if (locE != -1)
            {
              glUniform1i(locE, edgeEffect);
            }
            mag = 1;
            orien = 0;
            blur = 0;
            cout << "Image 1" << endl;
            picName = "image1-mandrill.png";
            PicGen(picName);
        }
        //When 2 is pressed display image 2
        else if(key == GLFW_KEY_2 && action == GLFW_PRESS)
        {
			blur = 0;
			GLint locB = glGetUniformLocation(shader.program,"blur");
            if (locB != -1)
            {
              glUniform1i(locB, blur);
            }
            oldPictureCenterX = pictureCenterX = 0;
            oldPictureCenterY = pictureCenterY = 0;
            colourEffect = 0;
            GLint locC = glGetUniformLocation(shader.program,"colourEffect");
            if (locC != -1)
            {
              glUniform1i(locC, colourEffect);
            }
            edgeEffect = 0;
            GLint locE = glGetUniformLocation(shader.program,"edgeEffect");
            if (locE != -1)
            {
              glUniform1i(locE, edgeEffect);
            }
            mag = 1;
            orien = 0;
            blur = 0;
            cout << "Image 2" << endl;
            picName = "image2-uclogo.png";
            PicGen(picName);
        }
        //When 3 is pressed display image 3
        else if(key == GLFW_KEY_3 && action == GLFW_PRESS)
        {
			blur = 0;
			GLint locB = glGetUniformLocation(shader.program,"blur");
            if (locB != -1)
            {
              glUniform1i(locB, blur);
            }
            oldPictureCenterX = pictureCenterX = 0;
            oldPictureCenterY = pictureCenterY = 0;
            colourEffect = 0;
            GLint locC = glGetUniformLocation(shader.program,"colourEffect");
            if (locC != -1)
            {
              glUniform1i(locC, colourEffect);
            }
            edgeEffect = 0;
            GLint locE = glGetUniformLocation(shader.program,"edgeEffect");
            if (locE != -1)
            {
              glUniform1i(locE, edgeEffect);
            }
            mag = 1;
            orien = 0;
            blur = 0;
            cout << "Image 3" << endl;
            picName = "image3-aerial.jpg";
            PicGen(picName);
        }
        //When 4 is pressed display image 4
        else if(key == GLFW_KEY_4 && action == GLFW_PRESS)
        {
			blur = 0;
			GLint locB = glGetUniformLocation(shader.program,"blur");
            if (locB != -1)
            {
              glUniform1i(locB, blur);
            }
            oldPictureCenterX = pictureCenterX = 0;
            oldPictureCenterY = pictureCenterY = 0;
            colourEffect = 0;
            GLint locC = glGetUniformLocation(shader.program,"colourEffect");
            if (locC != -1)
            {
              glUniform1i(locC, colourEffect);
            }
            edgeEffect = 0;
            GLint locE = glGetUniformLocation(shader.program,"edgeEffect");
            if (locE != -1)
            {
              glUniform1i(locE, edgeEffect);
            }
            orien = 0;
            mag = 1;
            blur = 0;
            cout << "Image 4" << endl;
            picName = "image4-thirsk.jpg";
            PicGen(picName);
        }
        //When 5 is pressed display image 5
        else if(key == GLFW_KEY_5 && action == GLFW_PRESS)
        {
			blur = 0;
			GLint locB = glGetUniformLocation(shader.program,"blur");
            if (locB != -1)
            {
              glUniform1i(locB, blur);
            }
            oldPictureCenterX = pictureCenterX = 0;
            oldPictureCenterY = pictureCenterY = 0;
            colourEffect = 0;
            GLint locC = glGetUniformLocation(shader.program,"colourEffect");
            if (locC != -1)
            {
              glUniform1i(locC, colourEffect);
            }
            edgeEffect = 0;
            GLint locE = glGetUniformLocation(shader.program,"edgeEffect");
            if (locE != -1)
            {
              glUniform1i(locE, edgeEffect);
            }
            orien = 0;
            mag = 1;
            blur = 0;
            cout << "Image 5" << endl;
            picName = "image5-pattern.png";
            PicGen(picName);
        }
        //When 6 is pressed display image 6
        else if(key == GLFW_KEY_6 && action == GLFW_PRESS)
        {
			blur = 0;
			GLint locB = glGetUniformLocation(shader.program,"blur");
            if (locB != -1)
            {
              glUniform1i(locB, blur);
            }
            oldPictureCenterX = pictureCenterX = 0;
            oldPictureCenterY = pictureCenterY = 0;
            colourEffect = 0;
            GLint locC = glGetUniformLocation(shader.program,"colourEffect");
            if (locC != -1)
            {
              glUniform1i(locC, colourEffect);
            }
            edgeEffect = 0;
            GLint locE = glGetUniformLocation(shader.program,"edgeEffect");
            if (locE != -1)
            {
              glUniform1i(locE, edgeEffect);
            }
            orien = 0;
            mag = 1;
            blur = 0;
            cout << "Image 6" << endl;
            picName = "image6-war.jpg";            
            if (!InitializeGeometry(&geometry))
                cout << "Program failed to intialize geometry!" << endl;
PicGen(picName);
            PicGen(picName);
        }
        //When 7 is pressed display image 7
        else if(key == GLFW_KEY_7 && action == GLFW_PRESS)
        {
			blur = 0;
			GLint locB = glGetUniformLocation(shader.program,"blur");
            if (locB != -1)
            {
              glUniform1i(locB, blur);
            }
            oldPictureCenterX = pictureCenterX = 0;
            oldPictureCenterY = pictureCenterY = 0;
            colourEffect = 0;
            GLint locC = glGetUniformLocation(shader.program,"colourEffect");
            if (locC != -1)
            {
              glUniform1i(locC, colourEffect);
            }
            edgeEffect = 0;
            GLint locE = glGetUniformLocation(shader.program,"edgeEffect");
            if (locE != -1)
            {
              glUniform1i(locE, edgeEffect);
            }
            orien = 0;
            mag = 1;
            blur = 0;
            cout << "Image 7" << endl;
            picName = "image7-mario.jpg";
            PicGen(picName);
        }
       //When 8 is pressed display image 8
        else if(key == GLFW_KEY_8 && action == GLFW_PRESS)
        {
			blur = 0;
			GLint locB = glGetUniformLocation(shader.program,"blur");
            if (locB != -1)
            {
              glUniform1i(locB, blur);
            }
            oldPictureCenterX = pictureCenterX = 0;
            oldPictureCenterY = pictureCenterY = 0;
            colourEffect = 0;
            GLint locC = glGetUniformLocation(shader.program,"colourEffect");
            if (locC != -1)
            {
              glUniform1i(locC, colourEffect);
            }
            edgeEffect = 0;
            GLint locE = glGetUniformLocation(shader.program,"edgeEffect");
            if (locE != -1)
            {
              glUniform1i(locE, edgeEffect);
            }
            orien = 0;
            mag = 1;
            blur = 0;
            cout << "Image 8" << endl;
            picName = "image8-coolGuy.jpeg";
            PicGen(picName);
        }
        //When r is pressed rotate with scrolling press again to magnify
         else if(key == GLFW_KEY_R && action == GLFW_PRESS)
        {
            rotateFlag++;
            if(rotateFlag==2)
                rotateFlag=0;

        }

        //When c is pressed apply relevent colour effect
        else if(key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            colourEffect++;
            if(colourEffect==6)
            {
                colourEffect = 0;
            }
            GLint locC = glGetUniformLocation(shader.program,"colourEffect");
            if (locC != -1)
            {
              glUniform1i(locC, colourEffect);
            }
            if(colourEffect==0)
            {
                cout << "Applying Default Colours" << endl;
            }
            if(colourEffect==1)
            {
                cout << "Applying First Greyscale" << endl;
            }
            else if(colourEffect==2)
            {
                cout << "Applying Second Greyscale" << endl;
            }
            else if(colourEffect==3)
            {
                cout << "Applying Third Greyscale" << endl;
            }
            else if(colourEffect==4)
            {
                cout << "Applying Sepia Tone" << endl;
            }
            else if(colourEffect==5)
            {
                cout << "Applying Negative Tone" << endl;
            }
            if (!InitializeGeometry(&geometry))
                                cout << "Program failed to intialize geometry!" << endl;
            PicGen(picName);
        }
        //When h is pressed apply horizontal sobel
        else if(key == GLFW_KEY_H && action == GLFW_PRESS)
        {
            edgeEffect = 1;
            GLint locE = glGetUniformLocation(shader.program,"edgeEffect");
            if (locE != -1)
            {
              glUniform1i(locE, edgeEffect);
            }
            float edgeMatrix[9]{
                -1.0, 0.0, 1.0,
                -2.0, 0.0, 2.0,
                -1.0, 0.0, 1.0
            };
            glUseProgram(shader.program);
            GLint edgeUniform = glGetUniformLocation(shader.program, "edge");
            glUniformMatrix3fv(edgeUniform, 1, GL_TRUE, edgeMatrix);

            cout << "Applying Horizontal Sobel Filter" << endl;
            if (!InitializeGeometry(&geometry))
                                cout << "Program failed to intialize geometry!" << endl;
            PicGen(picName);
        }
        //When v is pressed apply vertical sobel
        else if(key == GLFW_KEY_V && action == GLFW_PRESS)
        {
            edgeEffect = 1;
            GLint locE = glGetUniformLocation(shader.program,"edgeEffect");
            if (locE != -1)
            {
              glUniform1i(locE, edgeEffect);
            }
            float edgeMatrix[9]{
                1.0, 2.0, 1.0,
                0.0, 0.0, 0.0,
                -1.0, -2.0, -1.0,
            };
            glUseProgram(shader.program);
            GLint edgeUniform = glGetUniformLocation(shader.program, "edge");
            glUniformMatrix3fv(edgeUniform, 1, GL_TRUE, edgeMatrix);
            cout << "Applying Vertical Sobel Filter" << endl;
            if (!InitializeGeometry(&geometry))
                                cout << "Program failed to intialize geometry!" << endl;
            PicGen(picName);
        }
        //When U is pressed apply unsharp mask
        else if(key == GLFW_KEY_U && action == GLFW_PRESS)
        {
			cout << colourEffect << endl;
            edgeEffect = 2;
            GLint locE = glGetUniformLocation(shader.program,"edgeEffect");
            if (locE != -1)
            {
              glUniform1i(locE, edgeEffect);
            }
            float edgeMatrix[9]{
                0.0, -1.0, 0.0,
                -1.0, 5.0, -1.0,
                0.0, -1.0, 0.0
            };
            glUseProgram(shader.program);
            GLint edgeUniform = glGetUniformLocation(shader.program, "edge");
            glUniformMatrix3fv(edgeUniform, 1, GL_TRUE, edgeMatrix);
            cout << "Applying Unsharp Mask" << endl;
            if (!InitializeGeometry(&geometry))
                                cout << "Program failed to intialize geometry!" << endl;
            PicGen(picName);
        }
        //When g is pressed apply relevent gaussian blur
        else if(key == GLFW_KEY_G && action == GLFW_PRESS)
        {
            blur++;
            if(blur==4)
            {
                blur = 0;
            }
            GLint loc = glGetUniformLocation(shader.program,"blur");
            if (loc != -1)
            {
              glUniform1i(loc, blur);
            }
            if(blur==0)
            {
                cout << "Applying default level of blur" << endl;
            }
            GLint locB = glGetUniformLocation(shader.program,"blur");
            if (locB != -1)
            {
              glUniform1i(locB, blur);
            }
            if(blur==1)
            {
//                float blur1[9]{
//                    0.04, 0.12, 0.04,
//                    0.12, 0.36, 0.12,
//                    0.04, 0.12, 0.04
//                };
//                glUseProgram(shader.program);
//                GLint edgeUniform = glGetUniformLocation(shader.program, "blur1");
//                glUniformMatrix3fv(edgeUniform, 1, GL_TRUE, blur1);
                cout << "Applying 3x3 Gaussian Blur" << endl;
            }
            else if(blur==2)
            {
                cout << "Applying 5x5 Gaussian Blur" << endl;
            }
            else if(blur==3)
            {
                cout << "Applying 7x7 Gaussian Blur" << endl;
            }
            if (!InitializeGeometry(&geometry))
                                cout << "Program failed to intialize geometry!" << endl;
            PicGen(picName);
        }
}

//Key Position
static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        float windowWidth = 512.f;
        centerX = xpos/windowWidth;
        centerY = ypos/windowWidth;
        if(centerX<0.5)
        {
            centerX = centerX-1;
        }
        centerY = ((centerY*2)-1)*(-1);
}

//Mouse press
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        pressed = true;
    }
    else
    {
        if(pressed==true)
        {
            pictureCenterX = centerX;
            pictureCenterY = centerY;
        }
        pressed = false;
    }
    MyGeometry geometry;
    if (!InitializeGeometry(&geometry))
                        cout << "Program failed to intialize geometry!" << endl;
    PicGen(picName);
}

//Mouse scroll
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    glUseProgram(shader.program);
    MyGeometry geometry;
    if(rotateFlag==1)
    {
        cout << "Rotating Image" << endl;
        if(yoffset==1)
        {
            orien = orien + M_PI/32;
        }
        if(yoffset==-1)
        {
            orien = orien - M_PI/32;
        }
        if (!InitializeGeometry(&geometry))
                            cout << "Program failed to intialize geometry!" << endl;
    }

    else
    {
        cout << "Magnifying Image" << endl;
        if(yoffset==1)
        {
            mag = mag + 0.01;
        }
        if(yoffset==-1)
        {
            mag = mag - 0.01;
            if(mag<=0)
            {
                mag = 0.0001;
            }
        }
        if (!InitializeGeometry(&geometry))
                            cout << "Program failed to intialize geometry!" << endl;
    }
    PicGen(picName);
}



// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
        cout << "Welcome to Kool Kyle's \"Image Editing Studio\"!" << endl;
        cout << "Please refer to the readMe.txt file to see how the program works" << endl;
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// attempt to create a window with an OpenGL 4.1 core profile context
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        window = glfwCreateWindow(512, 512, "Kool Kyle's Assignment 2", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
        glfwSetCursorPosCallback(window, cursorPosCallback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwMakeContextCurrent(window);

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
//	MyShader shader;
	if (!InitializeShaders(&shader)) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}

        MyTexture texture;  //Moved, was after InitializeGeormety originally*********************
        //if(!InitializeTexture(&texture, "image7-mario.jpg", GL_TEXTURE_RECTANGLE))
            //cout << "Program failed to initialize geometry!" << endl;

        // call function to create and fill buffers with geometry data
        MyGeometry geometry;
        //if (!InitializeGeometry(&geometry))
          //      cout << "Program failed to intialize geometry!" << endl;

	// run an event-triggered main loop

	while (!glfwWindowShouldClose(window))
	{
		
		// call function to draw our scene
       // RenderScene(&geometry, &texture, &shader); //render scene with texture

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// clean up allocated resources before exit
        DestroyTexture(&texture);
        DestroyGeometry(&geometry);
        DestroyShaders(&shader);
	glfwDestroyWindow(window);
	glfwTerminate();

        cout << "Seeee yaaa!" << endl;
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file "
			<< filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}

void PicGen(std::string name)
{
            MyTexture texture;
            if(!InitializeTexture(&texture, name.c_str(), GL_TEXTURE_RECTANGLE))
                cout << "Program failed to initialize geometry!" << endl;

            // call function to create and fill buffers with geometry data
            MyGeometry geometry;
            if (!InitializeGeometry(&geometry))
                    cout << "Program failed to intialize geometry!" << endl;
//            MyShader shader;
//            if (!InitializeShaders(&shader))
//                cout << "Program could not initialize shaders, TERMINATING" << endl;
            RenderScene(&geometry, &texture, &shader);
}
