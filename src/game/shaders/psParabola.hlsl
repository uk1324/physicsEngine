/*
Finding the closest point P to a point on a parabola can be solved in 2 equivalent ways(they produce a cubic with the same roots just scaled differently, I think have not actually checked if the cubic is the same but it does have the same roots).
1. Find a point S(x, f(x)) that the slope of the line from P to S times the derivative of the parabola at the S.x = 0 (they are perpendicular).
Evaluate the distance function for each solution and find the one which is closest to P.
slope(S, P) * ddx(x) = -1
2. Minimize the function distance(S, P). Find the zeros of the derivative of distance. Evaluate the distance function for each solution and find the one which is closest to P.

Is there some third method that for example uses the fact that a parabola is the set of all points equally distant from a point and a line?

The method 1 is easier to calculate by hand because the derivative in method 2 is the scaled cubic divided by a big expression involving a square root. The bottom expression can be ignored becasue it doesn't change the zeros of the full expression. The bottom expression probably is undefined for P.x = S.x.
*/

/*
Newtons method failes if it tries to converge on some local minimum or if the derivative is nearly zero then the convergence is slow.
cbrt(x) is also not differentiable at zero so there are also problems with that. Newton's method doesn't work for non smooth functions.
Halley's method does the same thing so it also doesn't converge fast enough.
Computing the zeros of the cubic using Newton's method also breaks in the same place.
The compiler uses e^(ln(x) / 3.0) to compute the cube root.
TODO: Kantorovich theorem might be useful for locating the points where Newton's method failes. Then maybe use distance from a line with slope of the derivative near the non-converging region.
The method from Inigo Quilez has the same problems. On shadertoy they are not visible so it might be an issue with how DirectX computes pow.
Could try using the Tylor expansion of exp and ln to compute the sqrt and cbrt using doubles.
Example cubics around the region of error.
x^3 + 0.00392x + 0.35294
x^3 + 0.0549x + 0.37
When quadraticA = 1 then points in the region of error have around abs(p) < 0.0055. This region and the severity of the effect increases when a is decreased and decreases when a is increased. It might be possible to make it so this region stays the same be keeping a the same and scaling pos, but this would probably just do the same thing.
TODO: Maybe try the method described in "A rapidly convergent method for solving third-order polynomials".
There may be some way to scale it so when zoomed in the error doesn't appear because it is still solving with high precision.
*/

float cbrt(float x) {
	return (x < 0.0)
		? -pow(-x, 1.0 / 3.0)
		: pow(x, 1.0 / 3.0);
}

struct Complex{
	float x, y;
};

Complex makeComplex(float x, float y) {
	Complex r;
	r.x = x;
	r.y = y;
	return r;
}

Complex add(Complex a, Complex b) {
	return makeComplex(a.x + b.x, a.y + b.y);
}

Complex subtract(Complex a, Complex b) {
	return makeComplex(a.x - b.x, a.y - b.y);
}

Complex pow(Complex a, float p) {
	float length = sqrt(a.x * a.x + a.y * a.y);
	float angle = atan2(a.y, a.x);
	length = pow(length, p);
	float x = cos(p * angle) * length;
	float y = sin(p * angle) * length;
	return makeComplex(x, y);
}

Complex mul(Complex a, Complex b) {
	return makeComplex(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

float4 main(float2 texturePos : TexturePos, float3x2 transform : Transform, float invScale : InvScale, float3 color : Color, float3 info : Info) : SV_TARGET{
	// Instead of finding the distance to the parabola ax^2 could use x^2 then scale pos.x, solve and then unscale when calculating the distance. This is probably  faster because then some things would just reduce to 1.
	texturePos = texturePos - float2(0.5, 0.5);
	float2 pos = mul(float3(texturePos, 1.0), transform);
	pos -= info.yz;
	
	float2 p1 = pos;

	float a = info.x;

	float p = -2.0 * a * pos.y + 1;
	float q = -pos.x;
	float cubicA = 2.0 * a * a;
	// Depress the cubic.
	p /= cubicA;
	q /= cubicA;

	float delta = pow(q, 2.0f) / 4.0 + pow(p, 3.0) / 27.0;

	float2 closestPoint;
	float closestPointDistance;
	if (pos.x == 0) {
		closestPoint = float2(0.0, 0.0);
		closestPointDistance = pos.y;
	}
	// Solve x^3 + px + q using Cardano's formula.
	else if (delta < 0) {
		// casus irreducibilis - 3 real roots
		Complex sqrtDelta = pow(makeComplex(delta, 0.0), 1.0 / 2.0);
		Complex h = makeComplex(-(q / 2.0f), 0.0);
		Complex u0 = add(h, sqrtDelta);
		Complex u1 = subtract(h, sqrtDelta);
		Complex u0CubeRoot = pow(u0, 1.0 / 3.0);
		Complex u1CubeRoot = pow(u1, 1.0 / 3.0);

		// @Performance: The same root is always the closest one. Could dicard the imaginary part of the calculations.

		// x is a nth root of unity if x^n = 1
		Complex rootOfUnity0 = makeComplex(-0.5, sqrt(3.0) / 2.0);
		// This is the same as just squaring it.
		Complex rootOfUnity1 = makeComplex(-0.5, -rootOfUnity0.y);
		// 1 is the 3rd root of unity.

		// Result is always a real number.
		Complex result = add(u0CubeRoot, u1CubeRoot);
		closestPoint = float2(result.x, a * pow(result.x, 2.0));
		closestPointDistance = distance(closestPoint, pos);

		// Multiplying be the roots of unity just means finding the other cube root.
		result = add(mul(u0CubeRoot, rootOfUnity0), mul(u1CubeRoot, rootOfUnity1));
		float2 p = float2(result.x, a * pow(result.x, 2.0));
		float d = distance(p, pos);
		if (d < closestPointDistance) {
			closestPointDistance = d;
			closestPoint = p;
		}
		
		result = add(mul(u0CubeRoot, rootOfUnity1), mul(u1CubeRoot, rootOfUnity0));
		p = float2(result.x, a * pow(result.x, 2.0));
		d = distance(p, pos);
		if (d < closestPointDistance) {
			closestPointDistance = d;
			closestPoint = p;
		}

		// Instead of using the roots of unity could just divide by (first root - x) and solve the resulting quadratic.
	} else {
		// 1 real solution.
		float h = -(q / 2.0);
		float u0 = h + sqrt(delta);
		float u1 = h - sqrt(delta);
		float x = cbrt(u0) + cbrt(u1);

		closestPoint = float2(x, a * x * x);
		closestPointDistance = distance(closestPoint, pos);
	}

	invScale /= 4.0f;
	const float halfWidth = 0.003 * invScale;
	const float iterpolationWidth = 0.002 * invScale;

	float4 colorTransparent = float4(color, 0.0);
	return lerp(colorTransparent, float4(color, 1.0), smoothstep(halfWidth, halfWidth - iterpolationWidth, closestPointDistance));
}