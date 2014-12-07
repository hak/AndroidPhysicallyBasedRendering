//
//  ShaderPlain.vsh
//

#version 300 es

in highp vec3    myVertex;
in highp vec3    myNormal;
in mediump vec2  myUV;

out mediump vec2    texCoord;
out lowp    vec3    dynamicDiffuse;
out mediump vec3 eye;
out highp vec3 normal;
out highp vec3 halfvecLight0;

uniform highp mat4      uMVMatrix;
uniform highp mat4      uPMatrix;

uniform highp vec3      vLight0;
uniform highp vec3      vCamera;

uniform lowp vec3       vMaterialDiffuse;
uniform lowp vec3       vMaterialAmbient;
uniform lowp vec4       vMaterialSpecular;

void main(void)
{
    highp vec4 p = vec4(myVertex,1);
    gl_Position = uPMatrix * p;

    texCoord = myUV;
    highp vec3 worldNormal = vec3(mat3(uMVMatrix[0].xyz, uMVMatrix[1].xyz, uMVMatrix[2].xyz) * myNormal);

    normal = worldNormal;
    eye = -(uMVMatrix * p).xyz;
    halfvecLight0 = normalize(-vLight0) + normalize(eye);
    
    dynamicDiffuse = dot( worldNormal, normalize(-vLight0+eye) ) * vMaterialDiffuse  / 3.14f;
}
