//
//  ShaderSkybox.fsh
//

varying mediump vec3    texCoord;
uniform samplerCube sCubemapTexture;

void main()
{
  //gl_FragColor = vec4(1.0, 1.0, 1.0,1.0);
  
  
  gl_FragColor=textureCube(sCubemapTexture, texCoord);
// Gamma conversion
//	gl_FragColor.xyz = pow(gl_FragColor.xyz, vec3(1.0/1.8, 1.0/1.8, 1.0/1.8));
}
