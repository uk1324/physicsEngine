float CubeRoot(float d) {

	if (d < 0.0f) {

		return -pow(-d, 1.0 / 3.0);
	}

	else {

		return pow(d, 1.0 / 3.0);
	}
}
/*
Finding the closest point P to a point on a parabola can be solved in 2 equivalent ways(they produce a cubic with the same roots just scaled differently, I think have not actually checked if the cubic is the same but it does have the same roots).
1. Find a point S(x, f(x)) that the slope of the line from P to S times the derivative of the parabola at the S.x = 0 (they are perpendicular).
Evaluate the distance function for each solution and find the one which is closest to P.
slope(S, P) * ddx(x) = -1
2. Minimize the function distance(S, P). Find the zeros of the derivative of distance. Evaluate the distance function for each solution and find the one which is closest to P.

The method 1 is easier to calculate by hand because the derivative in method 2 is the scaled cubic divided by a big expression involving a square root. The bottom expression can be ignored becasue it doesn't change the zeros of the full expression. The bottom expression probably is undefined for P.x = S.x.
*/

bool IsNAN(float n)
{
	return n != n;
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

float4 main(float2 texturePos : TexturePos, float invScale : InvScale, float3 color : Color) : SV_TARGET {
	float2 pos = (texturePos - float2(0.5, 0.5)) * 8.0;
	pos.y = -pos.y;
	pos.x += 0.2;
	pos.y += 2.5;
	float2 p1 = pos;
	//return float4(pos, 0.0, 1.0);

	float a = 1.0;

	float4 col;
	if (pos.y < a * pos.x * pos.x) {
		col = float4(0.0f, 0.0f, 1.0f, 0.5f);
	} else {
		col = float4(1.0f, 0.0f, 0.0f, 0.5f);
	}


	//pos = float2(0.1, 0.2);

	float p = -2.0 * a * pos.y + 1;
	float q = -pos.x;
	float ca = 2.0 * a * a;
	p /= ca;
	q /= ca;
	float delta = pow(q, 2.0f) / 4.0 + pow(p, 3.0) / 27.0;

	//if (p1.y < pow(p1.x, 3.0) + p * p1.x + q) {
	//	col += float4(0.0f, 0.0f, 1.0f, 0.5f);
	//} else {
	//	col += float4(1.0f, 0.0f, 0.0f, 0.5f);
	//}
	//
	float2 closestPoint;
	float closestPointDistance;
	if (delta <= 0) {
		Complex sqrtDelta = pow(makeComplex(delta, 0.0), 1.0 / 2.0);
		Complex h = makeComplex(-(q / 2.0f), 0.0);
		Complex u0 = add(h, sqrtDelta);
		Complex u1 = subtract(h, sqrtDelta);
		Complex result = add(pow(u0, 1.0 / 3.0), pow(u1, 1.0 / 3.0));
		Complex rootOfUnity = makeComplex(-0.5, sqrt(3.0) / 2.0);
		//result = mul(result, rootOfUnity);
		//result = mul(result, rootOfUnity);
		//result = mul(result, rootOfUnity);
		float x0 = result.x;
		closestPoint = float2(x0, a * x0 * x0);
		closestPointDistance = distance(closestPoint, pos);
		//return float4(abs(result.y), 0.0, 0.0, 1.0);

		//Complex oldResult = result;
		//result = mul(result, rootOfUnity);
		//x0 = result.x;
		//float2 p0 = float2(x0, a * x0 * x0);
		//float d0 = distance(p0, pos);
		//if (d0 < closestPointDistance || abs(oldResult.y) > 0.05) {
		//	closestPointDistance = d0;
		//	closestPoint = p0;
		//}
		//
		//oldResult = result;
		//result = mul(result, rootOfUnity);
		//x0 = result.x;
		//float2 p1 = float2(x0, a * x0 * x0);
		//float d1 = distance(p1, pos);
		//if (d1 < closestPointDistance || abs(oldResult.y) > 0.05) {
		//	closestPointDistance = d1;
		//	closestPoint = p1;
		//}

		//return float4(0.0f, 1.0f, 1.0f, 1.0f);
		//float u0 = 





		{
			float b = x0;
			float c = pow(x0, 2.0) + p;
			float discriminant = pow(b, 2.0) - 4.0 * c;
			if (discriminant < 0.0) {
				closestPointDistance = distance(closestPoint, pos);
			} else {
				float discriminantSqrt = sqrt(discriminant);
				float x1 = (-b + discriminantSqrt) / 2.0;
				float x2 = (-b - discriminantSqrt) / 2.0;
		
				float2 p1 = float2(x1, a * x1 * x1);
				float d1 = distance(p1, pos);
				closestPointDistance = distance(closestPoint, pos);
				if (d1 < closestPointDistance) {
					closestPoint = p1;
					closestPointDistance = d1;
				} 
				float2 p2 = float2(x2, a * x2 * x2);
				float d2 = distance(p2, pos);
				if (d2 < closestPointDistance) {
					closestPoint = p2;
					closestPointDistance = d2;
				}
			}
		}
	} else {
		float u0 = -(q / 2.0f) + sqrt(delta);
		float u1 = -(q / 2.0f) - sqrt(delta);
		//float x0 = pow(u0, 1.0 / 3.0) + pow(u1, 1.0 / 3.0);

		float x0 = CubeRoot(u0) + CubeRoot(u1);

		closestPoint = float2(x0, a * x0 * x0);
		closestPointDistance = distance(closestPoint, pos);
	}


	//else {
	////	return float4(1.0f, 0.0f, 0.0f, 1.0f);
	//}
	//return float4(float3(delta, delta, delta), 1.0f);
	//if (pos.y < pow(pos.x, 3.0) * p + pos.x * q) {
	//	return float4(0.0f, 0.0f, 1.0f, 1.0f);
	//} else {
	//	return float4(0.0f, 1.0f, 0.0f, 1.0f);
	//}

	//if (IsNAN(sqrt(-1.0))) {
	//	return float4(0.0f, 0.0f, 1.0f, 1.0f);
	//}

	//if (delta <= 0) {
	//	return float4(0.0f, 0.0f, 1.0f, 1.0f);
	//} else {
	//	return float4(1.0f, 0.0f, 0.0f, 1.0f);
	//}
	//{
	//	float b = x0;
	//	float c = pow(x0, 2.0) + p;
	//	float discriminant = pow(b, 2.0) - 4.0 * c;
	//	if (discriminant < 0.0) {
	//		closestPointDistance = distance(closestPoint, pos);
	//	} else {
	//		float discriminantSqrt = sqrt(discriminant);
	//		float x1 = (-b + discriminantSqrt) / 2.0;
	//		float x2 = (-b - discriminantSqrt) / 2.0;
	//
	//		float2 p1 = float2(x1, a * x1 * x1);
	//		float d1 = distance(p1, pos);
	//		closestPointDistance = distance(closestPoint, pos);
	//		if (d1 < closestPointDistance) {
	//			closestPoint = p1;
	//			closestPointDistance = d1;
	//		} 
	//		float2 p2 = float2(x2, a * x2 * x2);
	//		float d2 = distance(p2, pos);
	//		if (d2 < closestPointDistance) {
	//			closestPoint = p2;
	//			closestPointDistance = d2;
	//		}
	//	}
	//}

	//if (isnan(closestPoint.y)) {
	//	return float4(0.0f, 0.0f, 1.0f, 1.0f);
	//}

	//return float4(closestPoint, 0.0, 1.0);

	//if (closestPointDistance < 0.5) {
	//	return float4(0.0f, 1.0f, 0.0f, 1.0f);
	//} else {
	//	return float4(0.0f, 0.0f, 1.0f, 1.0f);
	//}

	//if (distance(p1, closestPoint) < 0.05) {
	//	return float4(0.0, 1.0, 1.0, 1.0);
	//}

	//if (distance(p1, pos) < 0.05) {
	//	return float4(1.0, 1.0, 1.0, 1.0);
	//}
	//return col;

	//return col * closestPointDistance;

	return float4(float3(closestPointDistance, closestPointDistance, closestPointDistance), 1.0);

	return float4(0.0f, 1.0f, 0.0f, 1.0f);
	/*float deltaQuadratic = */

	/*float3 coefficients = float3(1.0, 0, 0.2);

	float2 pos = (texturePos - float2(0.5, 0.5)) * 2.0;
	pos.y = -pos.y;
	float4 col;
	if (coefficients[0] * pow(pos.x, 2.0) + coefficients[1] * pos.x + coefficients[2] < pos.y) {
		col = float4(1.0f, 1.0f, 1.0f, 1.0f);
	} else {
		col = float4(1.0f, 0.0f, 0.0f, 1.0f);
	}

	//float2 p = pos;

	//pos = float2(0.2, 0.0);


	float
		a = 2.0 * pow(coefficients[0], 2.0),
		b = 3.0 * coefficients[0] * coefficients[1],
		c = pow(coefficients[1], 2.0) + 2.0 * coefficients[0] * coefficients[2] - 2.0 * a * pos.y + 1,
		d = coefficients[1] * coefficients[2] - pos.y * coefficients[1] - pos.x;
	//float a = 1.0, b = 1.0, c = 1.0, d = 0.0;

	float delta0 = pow(b, 2.0) - 3.0 * a * c;
	float delta1 = 2.0 * pow(b, 3.0) - 9.0 * a * b * c + 27.0 * pow(a, 2.0) * d;
	float C = CubeRoot(delta1 + sqrt(pow(delta1, 2.0) - 4.0 * pow(delta0, 3.0)) / 2.0);

	bool test = false;
	if (abs(C) < 0.001) {
		C = CubeRoot(delta1 - sqrt(pow(delta1, 2.0) - 4.0 * pow(delta0, 3.0)) / 2.0);
		test = true;
		//if (abs(C) < 0.01) {
		//	test = true;
		//}
	}

	float x0 = -(1.0 / (3.0 * a)) * (b + C + delta0 / C);
	float x1 = 1000000000;
	if (!test) {
		C = CubeRoot(delta1 - sqrt(pow(delta1, 2.0) - 4.0 * pow(delta0, 3.0)) / 2.0);
		x1 = -(1.0 / (3.0 * a)) * (b + C + delta0 / C);
	}

	float x = x0;
	float y0 = pow(x, 2.0) * coefficients[0] + x * coefficients[1] + coefficients[2];
	x = x1;
	float y1 = pow(x, 2.0) * coefficients[0] + x * coefficients[1] + coefficients[2];

	float2 p0 = float2(x0, y0);
	float2 p1 = float2(x1, y1);
	float2 p;
	
	if (isnan(x0) || isnan(x1)) {
		return float4(0.0f, 1.0f, 0.0f, 1.0f);
	}

	if (distance(p0, pos) < distance(p1, pos)) {
		p = p0;
	} else {
		p = p1;
	}

	if (distance(p, pos) < 0.2) {
		return float4(1.0f, 1.0f, 1.0f, 1.0f);
	} else {
		return float4(1.0f, 0.0f, 0.0f, 1.0f);
	}

	//if (distance(float2(x, y), p) < 0.01) {
	//	col = float4(0.0f, 0.0f, 1.0f, 1.0f);
	//} 
	//
	//if (distance(p, pos) < 0.01) {
	//	col = float4(0.0f, 1.0f, 0.0f, 1.0f);
	//}

	return col;


	//if (a * pow(pos.x, 3.0) + b * pow(pos.x, 2.0) + c * pos.x + d < pos.y) {
	//	return float4(1.0f, 1.0f, 1.0f, 1.0f);
	//} else {
	//	return float4(1.0f, 0.0f, 0.0f, 1.0f);
	//}


	//float depress = 

	//float p = (3.0 * a * c - pow(b, 2.0)) / (3.0 * pow(a, 2.0));
	//float q = (2.0 * pow(b, 3.0) - 9.0 * a * b * c + 27.0 * pow(a, 2.0) * d) / (27.0 * pow(a, 3.0));

	//float delta = pow(q, 2.0) / 4.0 + pow(p, 3.0) / 27.0;

	//float u0 = -q / 2.0 + sqrt(delta);
	//float u1 = -q / 2.0 + sqrt(delta);

	//float t = pow(u0, 1.0 / 3.0) + pow(u1, 1.0 / 3.0);

	//float x = t - (b / (3.0 * a));

	//float y = coefficients[0] * pow(x, 2.0) + coefficients[1] * x + coefficients[2];

//	if (distance(float2(x, y), pos) < 0.5) {
//		return float4(1.0f, 1.0f, 1.0f, 1.0f);
//	} else {
//		return float4(1.0f, 0.0f, 0.0f, 1.0f);
//	}

	//b /= a;
	//c /= a;
	//d /= a;

	//float 
	//	d0 = pow(b, 2.0) - 3 * a * c
	//	d1 = 2 * pow(b, 3.0) - 9 * a * b * c + 27 * pow(a, 2.0) * d;
	//
	//float C = pow(1.0 / 3.0f);
	//float x = -(1.0 / (3.0f * a)) * (b )
	//
	//
	////return float4(pos, 0.0, 1.0);
	//
	//eturn float4(1.0f, 1.0f, 1.0f, 1.0f);
	*/
}