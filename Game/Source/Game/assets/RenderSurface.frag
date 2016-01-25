#version 150

in vec2 Texcoord;

out vec4 outColor;

uniform sampler2D texFramebuffer;

void main() {
	vec4 c = texture(texFramebuffer, Texcoord);
	float cd = abs(c.r - Texcoord.y);
	c.r += (c.r/cd - c.r)*Texcoord.y;
	
	c.g += ((c.g/Texcoord.y) - c.g) * 0.1;
	
	cd = 1;//c.b - Texcoord.y;
	c.b += (c.r/cd - c.r)*Texcoord.y;
	
	if(Texcoord.y > 0.5f){
		c.rgb += Texcoord.y - 0.5f;
	}
    outColor = vec4(c);
}
