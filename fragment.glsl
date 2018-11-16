// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// interpolated colour received from vertex stage
in vec3 Colour;

//interpolated texture coordinates
in vec2 textureCoords;

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

//Our texture to read from
uniform sampler2DRect tex;
uniform int colourEffect;
uniform mat3 edge;
uniform int edgeEffect;

uniform int blur;
//uniform mat3 blur1;
//uniform mat5 blur2;
//uniform mat7 blur3;


void main(void)
{
    float L;
    int res = 1;
    vec2 newCoords;
    newCoords.x = res * (int(textureCoords.x)/res);
    newCoords.y = res * (int(textureCoords.y)/res);
    vec4 colour = texture(tex, newCoords);          //Current Coordinates


	//Colour Effects
    if(colourEffect==1)
    {
      L = (colour.r*0.333) + (colour.g*0.333) + (colour.b*0.333);     //First grey
      colour.r = L;
      colour.g = L;
      colour.b = L;
    }
    else if(colourEffect==2)
    {
      L = (colour.r*0.299) + (colour.g*0.587) + (colour.b*0.114);     //Second grey
      colour.r = L;
      colour.g = L;
      colour.b = L;
    }
    else if(colourEffect==3)
    {
      L = (colour.r*0.213) + (colour.g*0.715) + (colour.b*0.072);     //Third grey
      colour.r = L;
      colour.g = L;
      colour.b = L;
    }
    else if(colourEffect==4) 							      	      //Sepia tone
    {
      colour.r = (colour.r*0.393) + (colour.g*0.769) + (colour.b*0.189);
      colour.g = (colour.r*0.349) + (colour.g*0.686) + (colour.b*0.168);
      colour.b = (colour.r*0.272) + (colour.g*0.534) + (colour.b*0.131);
    }
    else if(colourEffect==5)
    {
      colour.r = 1-colour.r;
      colour.g = 1-colour.g;
      colour.b = 1-colour.b;
    }


    if((edgeEffect==1)||(edgeEffect==2))
    {
      //Edge Effects
      vec2 tempCoords=vec2(0,0);
      vec4 edgePixel=vec4(0,0,0,0);
      vec4 change=vec4(0,0,0,0);
      vec4 finalPixel=vec4(0,0,0,0);
      int k = 0;
      int h = 2;
      for(int i=-1;i<=1;i++)
      {
        for(int j=-1;j<=1;j++)
        {
          tempCoords = newCoords + vec2(i,j);
          edgePixel =  texture(tex, tempCoords);
          change = edge[h][k]*edgePixel;
          finalPixel += change;
          h--;
        }
        k++;
        h=2;
      }
      if(edgeEffect==1)
      {
        colour = abs(finalPixel);
      }
      else
      {
        colour = finalPixel;
      }
    }



	//Gaussian Blue
	if(blur==1)  //Done the easy way
  {
    vec2 tempCoords=vec2(0,0);
    vec4 edgePixel=vec4(0,0,0,0);
    vec4 change=vec4(0,0,0,0);
    vec4 finalPixel=vec4(0,0,0,0);
    vec3 blur1r=vec3(0.2,0.6,0.2);
    vec3 blur1c=vec3(0.2,0.6,0.2);
    int k = 0;
    int h = 2;
    for(int i=-1;i<=1;i++)
    {
      for(int j=-1;j<=1;j++)
      {
        tempCoords = newCoords + vec2(i,j);
        edgePixel =  texture(tex, tempCoords);
        change = blur1r[h]*blur1r[k]*edgePixel;
        finalPixel += change;
        h--;
      }
      k++;
      h=2;
    }
    colour = finalPixel;
	}
	else if(blur==2) //Then the formula way
	{
    vec2 tempCoords=vec2(0,0);
    vec4 edgePixel=vec4(0,0,0,0);
    vec4 change=vec4(0,0,0,0);
    vec4 finalPixel=vec4(0,0,0,0);
    float gaussRow[5] = float[5](0.06,0.24,0.4,0.24,0.06);
    int k = 0;
    int h = 4;
    for(int i=-2;i<=2;i++)
    {
      for(int j=-2;j<=2;j++)
      {
        tempCoords = newCoords + vec2(i,j);
        edgePixel =  texture(tex, tempCoords);
        change = gaussRow[h]*gaussRow[k]*edgePixel;
        finalPixel += change;
        h--;
      }
      k++;
      h=4;
    }
    colour = finalPixel;
  }

	else if(blur==3)
	{
  vec2 tempCoords=vec2(0,0);
  vec4 edgePixel=vec4(0,0,0,0);
  vec4 change=vec4(0,0,0,0);
  vec4 finalPixel=vec4(0,0,0,0);
  float gaussRow[7] = float[7](0.004,0.054,0.242,0.4,0.242,0.054,0.004);
  int k = 0;
  int h = 6;
  for(int i=-3;i<=3;i++)
  {
    for(int j=-3;j<=3;j++)
    {
      tempCoords = newCoords + vec2(i,j);
      edgePixel =  texture(tex, tempCoords);
      change = gaussRow[h]*gaussRow[k]*edgePixel;
      finalPixel += change;
      h--;
    }
    k++;
    h=6;
  }
  colour = finalPixel;
  }

  FragmentColour = vec4(colour);
}
