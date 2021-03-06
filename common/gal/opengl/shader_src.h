// Do not edit this file, it is autogenerated by CMake.

#ifndef SHADER_SRC_H
#define SHADER_SRC_H

const unsigned int shaders_number = 2;
const char* shaders_src[] = 
{

// /vault/home/brian/git/kicad-source-mirror/common/gal/opengl/shader.vert
"#version 120\n"
"const float SHADER_LINE=1.0;\n"
"const float SHADER_FILLED_CIRCLE=2.0;\n"
"const float SHADER_STROKED_CIRCLE=3.0;\n"
"const float SHADER_FONT=4.0;\n"
"const float MIN_WIDTH=1.0;\n"
"attribute vec4 attrShaderParams;\n"
"varying vec4 shaderParams;\n"
"varying vec2 circleCoords;\n"
"void main()\n"
"{\n"
"shaderParams=attrShaderParams;\n"
"if(shaderParams[0]==SHADER_LINE)\n"
"{\n"
"float lineWidth=shaderParams[3];\n"
"float worldScale=gl_ModelViewMatrix[0][0];\n"
"if(worldScale*lineWidth<MIN_WIDTH)\n"
"gl_Position=gl_ModelViewProjectionMatrix*\n"
"(gl_Vertex+vec4(shaderParams.yz*MIN_WIDTH/(worldScale*lineWidth),0.0,0.0));\n"
"else\n"
"gl_Position=gl_ModelViewProjectionMatrix*\n"
"(gl_Vertex+vec4(shaderParams.yz,0.0,0.0));\n"
"}\n"
"else if((shaderParams[0]==SHADER_STROKED_CIRCLE)||\n"
"(shaderParams[0]==SHADER_FILLED_CIRCLE))\n"
"{\n"
"if(shaderParams[1]==1.0)\n"
"circleCoords=vec2(-sqrt(3.0),-1.0);\n"
"else if(shaderParams[1]==2.0)\n"
"circleCoords=vec2(sqrt(3.0),-1.0);\n"
"else if(shaderParams[1]==3.0)\n"
"circleCoords=vec2(0.0,2.0);\n"
"else if(shaderParams[1]==4.0)\n"
"circleCoords=vec2(-3.0/sqrt(3.0),0.0);\n"
"else if(shaderParams[1]==5.0)\n"
"circleCoords=vec2(3.0/sqrt(3.0),0.0);\n"
"else if(shaderParams[1]==6.0)\n"
"circleCoords=vec2(0.0,2.0);\n"
"float lineWidth=shaderParams[3];\n"
"float worldScale=gl_ModelViewMatrix[0][0];\n"
"if(worldScale*lineWidth<MIN_WIDTH)\n"
"shaderParams[3]=shaderParams[3]/(worldScale*lineWidth);\n"
"gl_Position=ftransform();\n"
"}\n"
"else\n"
"{\n"
"gl_Position=ftransform();\n"
"}\n"
"gl_FrontColor=gl_Color;\n"
"}\n"
,
// /vault/home/brian/git/kicad-source-mirror/common/gal/opengl/shader.frag
"#version 120\n"
"const int FONT_TEXTURE_WIDTH=1024;\n"
"const int FONT_TEXTURE_HEIGHT=1024;\n"
"#define USE_MSDF\n"
"const float SHADER_LINE=1.0;\n"
"const float SHADER_FILLED_CIRCLE=2.0;\n"
"const float SHADER_STROKED_CIRCLE=3.0;\n"
"const float SHADER_FONT=4.0;\n"
"varying vec4 shaderParams;\n"
"varying vec2 circleCoords;\n"
"uniform sampler2D fontTexture;\n"
"void filledCircle(vec2 aCoord)\n"
"{\n"
"if(dot(aCoord,aCoord)<1.0)\n"
"gl_FragColor=gl_Color;\n"
"else\n"
"discard;\n"
"}\n"
"void strokedCircle(vec2 aCoord,float aRadius,float aWidth)\n"
"{\n"
"float outerRadius=aRadius+(aWidth/2);\n"
"float innerRadius=aRadius -(aWidth/2);\n"
"float relWidth=innerRadius/outerRadius;\n"
"if((dot(aCoord,aCoord)<1.0)&&\n"
"(dot(aCoord,aCoord)>relWidth*relWidth))\n"
"gl_FragColor=gl_Color;\n"
"else\n"
"discard;\n"
"}\n"
"#ifdef USE_MSDF\n"
"float median(vec3 v)\n"
"{\n"
"return max(min(v.r,v.g),min(max(v.r,v.g),v.b));\n"
"}\n"
"#endif\n"
"void main()\n"
"{\n"
"if(shaderParams[0]==SHADER_FILLED_CIRCLE)\n"
"{\n"
"filledCircle(circleCoords);\n"
"}\n"
"else if(shaderParams[0]==SHADER_STROKED_CIRCLE)\n"
"{\n"
"strokedCircle(circleCoords,shaderParams[2],shaderParams[3]);\n"
"}\n"
"else if(shaderParams[0]==SHADER_FONT)\n"
"{\n"
"vec2 tex=shaderParams.yz;\n"
"float derivative=length(dFdx(tex))*FONT_TEXTURE_WIDTH/8;\n"
"#ifdef USE_MSDF\n"
"float dist=median(texture2D(fontTexture,tex).rgb);\n"
"#else\n"
"float dist=texture2D(fontTexture,tex).r;\n"
"#endif\n"
"float alpha=smoothstep(0.5 - derivative,0.5+derivative,dist);\n"
"gl_FragColor=vec4(gl_Color.rgb,alpha);\n"
"}\n"
"else\n"
"{\n"
"gl_FragColor=gl_Color;\n"
"}\n"
"}\n"
,};
#endif /* SHADER_SRC_H */