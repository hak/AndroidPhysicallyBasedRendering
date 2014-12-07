//
//  ShaderPlain.vsh
//

attribute highp vec3    myVertex;
varying mediump vec3    texCoord;
uniform highp mat4      uPMatrix;

void main(void)
{
  highp vec4 p = vec4(myVertex,1);
  p = uPMatrix * p;
  gl_Position = p.xyww;

  texCoord = myVertex;//p.xyz;
}
