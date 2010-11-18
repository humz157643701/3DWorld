varying vec3 normal;

void main()
{
	vec3 normal2 = normalize(normal); // renormalize
	vec4 color = gl_FrontMaterial.emission + gl_FrontMaterial.ambient * gl_LightModel.ambient;
	if (enable_light0) color += add_light_comp(normal2, 0);
	if (enable_light1) color += add_light_comp(normal2, 1);
	gl_FragColor = color;
}
