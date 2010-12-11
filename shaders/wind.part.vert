uniform float time   = 0.0;
uniform float wind_x = 0.0;
uniform float wind_y = 0.0;
uniform sampler2D tex_noise;

float get_wind_delta(in vec3 vertex) {
	float tx  = (time*wind_x + 1.2*vertex.x);
	float ty  = (time*wind_y + 1.2*vertex.y);
	float mag = texture2D(tex_noise, vec2(tx, ty)).a;
	float flexibility = 0.75*gl_Color.g + 0.25; // burned gras/leavess are less flexible
	return flexibility * mag;
} 
