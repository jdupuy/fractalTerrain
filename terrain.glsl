#version 330

// grid params
uniform float uGridScale;

// light and viewing params
uniform vec3 uLightPos;
uniform vec3 uEyePos;
uniform mat4 uModelViewProjection;

// noise params
uniform float uH;
uniform float uLacunarity;
uniform int uOctaves;
uniform float uHeightScale;


// functions
uniform vec4 pParam = vec4(289.0, 34.0, 1.0, 7.0);
// Example constant with a 289 element permutation
//const vec4 pParam = vec4( 17.0*17.0, 34.0, 1.0, 7.0);

vec3 permute(vec3 x0,vec3 p) { 
	vec3 x1 = mod(x0 * p.y, p.x);
	return floor(  mod( (x1 + p.z) *x0, p.x ));
}

float simplex_noise2(vec2 v) {
	const vec2 C = vec2(0.211324865405187134, // (3.0-sqrt(3.0))/6.;
		                0.366025403784438597); // 0.5*(sqrt(3.0)-1.);
	const vec3 D = vec3( 0., 0.5, 2.0) * 3.14159265358979312;

	// First corner
	vec2 i  = floor(v + dot(v, C.yy) );
	vec2 x0 = v -   i + dot(i, C.xx);

	// Other corners
	vec2 i1  =  (x0.x > x0.y) ? vec2(1.,0.) : vec2(0.,1.) ;

	//  x0 = x0 - 0. + 0. * C
	vec2 x1 = x0 - i1 + 1. * C.xx ;
	vec2 x2 = x0 - 1. + 2. * C.xx ;

	// Permutations
	i = mod(i, pParam.x);
	vec3 p = permute( permute( 
		     i.y + vec3(0., i1.y, 1. ), pParam.xyz)
		   + i.x + vec3(0., i1.x, 1. ), pParam.xyz);

	// ( N points uniformly over a line, mapped onto a diamond.)
	vec3 x = fract(p / pParam.w) ;
	vec3 h = 0.5 - abs(x) ;

	vec3 sx = vec3(lessThan(x,D.xxx)) *2. -1.;
	vec3 sh = vec3(lessThan(h,D.xxx));

	vec3 a0 = x + sx*sh;
	vec2 p0 = vec2(a0.x,h.x);
	vec2 p1 = vec2(a0.y,h.y);
	vec2 p2 = vec2(a0.z,h.z);
	vec3 g = 2.0 * vec3( dot(p0, x0), dot(p1, x1), dot(p2, x2) );

	// mix
	vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x1,x1), dot(x2,x2)), 0.);
	m = m*m ;
	return 1.66666* 70.*dot(m*m, g);
}


// procedural fBm (p.437)
float fBm(vec2 p, float H, float lacunarity, int octaves) {
	float value = 0.0;
	for(int i=0;i<octaves; ++i) {
		value+= simplex_noise2(p) * pow(lacunarity, -H*i);
		p*= lacunarity;
	}
	return value;
}

// simple multifractal (p.440)
float fBm2(vec2 p, float H, float lacunarity, int octaves, float offset) {
	float value = 1.0;
	for(int i=0;i<octaves; ++i) {
		value*= (simplex_noise2(p)+offset) * pow(lacunarity, -H*i);
		p*= lacunarity;
	}
	return value;
}

// ridged multifractal terrain (p.504)
float fBm3(vec2 p, float H, float lacunarity, int octaves, float offset, float gain) {
	float result, frequency, signal, weight;

	frequency = 1.0;

	signal = offset - abs(simplex_noise2(p));
	signal*= signal;
	result = signal;
	weight = 1.0;

	for(int i=1; i<octaves; ++i) {
		p*= lacunarity;
		weight = clamp(signal*gain, 0.0,1.0);
		signal = offset - abs(simplex_noise2(p));
		signal*= signal*weight;
		result+= signal * pow(frequency, -H);
		frequency*= lacunarity;
	}

	return result;

}


#ifdef _VERTEX_
layout(location=0) in vec2 iPosition;

out vec3 vsPosition;
#define oPosition vsPosition

void main() {
	vec4 p = vec4(iPosition*uGridScale, 0, 1);
//	p.z = fBm(p.xy, uH, uLacunarity, uOctaves)*uHeightScale;
//	p.z = fBm2(p.xy, uH, uLacunarity, uOctaves, 0.8)*uHeightScale;
	p.z = fBm3(p.xy, uH, uLacunarity, uOctaves, 1.0, 2.0)*uHeightScale;
	oPosition = p.xzy;

	gl_Position = uModelViewProjection * p.xzyw;
}
#endif // _VERTEX_


#ifdef _FRAGMENT_
in vec3 vsPosition; 
layout(location=0) out vec4 oColour;

#define iPosition vsPosition

void main() {
	vec3 L = normalize(uLightPos - iPosition);
	vec3 V = normalize(uEyePos - iPosition);
	vec3 N = normalize(cross(dFdx(iPosition), dFdy(iPosition)));

	// do some procedural shading
	float phong = max(dot(N,L),0.0);

	const vec3 ROCK = vec3(0.6,0.2,0.0);
	const vec3 GRASS = vec3(0.1,0.6,0.0);
	const vec3 SNOW  = vec3(1.0);

	oColour.rgb = phong*mix(ROCK, SNOW, smoothstep(0.04,0.07,iPosition.y));
}

#endif // _FRAGMENT_


