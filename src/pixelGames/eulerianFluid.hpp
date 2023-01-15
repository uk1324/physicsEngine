#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>
#include <math/vec2.hpp>

#include <bitset>






class Fluid
{
public:
	Fluid(double _density, int _numX, int _numY, double _h, double _overRelaxation = 1.9, int _numThreads = 4);

	// ----------------- start of simulator ------------------------------
	void integrate(double dt, double gravity);
	void solveIncompressibility(int numIters, double dt);
	void extrapolate();
	double sampleField(double x, double y, int field);
	double avgU(int i, int j);
	double avgV(int i, int j);
	void advectVelocity(double dt);
	void advectTracer(double dt);
	void simulate(double dt, double gravity, int numIters);
	// ----------------- end of simulator ------------------------------

	void updateFluidParameters();
	template<typename T>
	auto at(std::vector<T>& vec, i64 x, i64 y) -> T&;

	double density;
	int numX;
	int numY;
	int numCells;
	int num;
	double h;
	double overRelaxation;
	std::vector<double> u;
	std::vector<double> v;
	std::vector<double> Vel;
	std::vector<double> newU;
	std::vector<double> newV;
	std::vector<double> pressure;
	std::vector<bool> isWall;
	std::vector<double> s;
	std::vector<double> m;
	std::vector<double> newM;
};




//struct Fluid {
//	float density;
//	float numX;
//	float numY;
//	float numCells;
//	float h;
//	std::vector<float> u;
//	std::vector<float> v;
//	std::vector<float> newU;
//	std::vector<float> newV;
//	std::vector<float> p;
//	std::vector<float> s;
//	std::vector<float> m;
//	std::vector<float> newM;
//
//	Fluid(float density, int numX, int numY, float h) {
//		this->density = density;
//		this->numX = numX;
//		this->numY = numY;
//		this->numCells = this->numX * this->numY;
//		this->h = h;
//		this->u = std::vector<float>(this->numCells);
//		this->v = std::vector<float>(this->numCells);
//		this->newU = std::vector<float>(this->numCells);
//		this->newV = std::vector<float>(this->numCells);
//		this->p = std::vector<float>(this->numCells);
//		this->s = std::vector<float>(this->numCells);
//		this->m = std::vector<float>(this->numCells);
//		this->newM = std::vector<float>(this->numCells);
//		for (auto& v : m)
//			v = 1.0f;
//		/*this->m.fill(1.0)
//		var num = numX * numY;*/
//
//		for (auto i = 0; i < this->numX; i++) {
//			for (auto j = 0; j < this->numY; j++) {
//				auto s = 1.0;	// fluid
//				if (i == 0 || i == this->numX - 1 || j == 0 || j == this->numY - 1)
//					s = 0.0;	// solid
//				this->s[i * this->numY + j] = s;
//			}
//		}
//	}
//
//	void integrate(float dt, float gravity) {
//		auto n = this->numY;
//		for (auto i = 1; i < this->numX; i++) {
//			for (auto j = 1; j < this->numY - 1; j++) {
//				if (this->s[i * n + j] != 0.0 && this->s[i * n + j - 1] != 0.0)
//					this->v[i * n + j] += gravity * dt;
//			}
//		}
//	}
//
//	void solveIncompressibility(float numIters, float dt) {
//		auto n = this->numY;
//		auto cp = this->density * this->h / dt;
//
//		for (auto iter = 0; iter < numIters; iter++) {
//
//			for (auto i = 1; i < this->numX - 1; i++) {
//				for (auto j = 1; j < this->numY - 1; j++) {
//
//					if (this->s[i * n + j] == 0.0)
//						continue;
//
//					//auto s = this->s[i * n + j];
//					auto sx0 = this->s[(i - 1) * n + j];
//					auto sx1 = this->s[(i + 1) * n + j];
//					auto sy0 = this->s[i * n + j - 1];
//					auto sy1 = this->s[i * n + j + 1];
//					auto s = sx0 + sx1 + sy0 + sy1;
//					if (s == 0.0)
//						continue;
//
//					auto div = this->u[(i + 1) * n + j] - this->u[i * n + j] +
//						this->v[i * n + j + 1] - this->v[i * n + j];
//
//					auto p = -div / s;
//					p *= 1.9f;
//					this->p[i * n + j] += cp * p;
//
//					this->u[i * n + j] -= sx0 * p;
//					this->u[(i + 1) * n + j] += sx1 * p;
//					this->v[i * n + j] -= sy0 * p;
//					this->v[i * n + j + 1] += sy1 * p;
//				}
//			}
//		}
//	}
//
//	//extrapolate() {
//	//	var n = this.numY;
//	//	for (var i = 0; i < this.numX; i++) {
//	//		this.u[i * n + 0] = this.u[i * n + 1];
//	//		this.u[i * n + this.numY - 1] = this.u[i * n + this.numY - 2];
//	//	}
//	//	for (var j = 0; j < this.numY; j++) {
//	//		this.v[0 * n + j] = this.v[1 * n + j];
//	//		this.v[(this.numX - 1) * n + j] = this.v[(this.numX - 2) * n + j]
//	//	}
//	//}
//
//	//sampleField(x, y, field) {
//	//	var n = this.numY;
//	//	var h = this.h;
//	//	var h1 = 1.0 / h;
//	//	var h2 = 0.5 * h;
//
//	//	x = Math.max(Math.min(x, this.numX * h), h);
//	//	y = Math.max(Math.min(y, this.numY * h), h);
//
//	//	var dx = 0.0;
//	//	var dy = 0.0;
//
//	//	var f;
//
//	//	switch (field) {
//	//	case U_FIELD: f = this.u; dy = h2; break;
//	//	case V_FIELD: f = this.v; dx = h2; break;
//	//	case S_FIELD: f = this.m; dx = h2; dy = h2; break
//
//	//	}
//
//	//	var x0 = Math.min(Math.floor((x - dx) * h1), this.numX - 1);
//	//	var tx = ((x - dx) - x0 * h) * h1;
//	//	var x1 = Math.min(x0 + 1, this.numX - 1);
//
//	//	var y0 = Math.min(Math.floor((y - dy) * h1), this.numY - 1);
//	//	var ty = ((y - dy) - y0 * h) * h1;
//	//	var y1 = Math.min(y0 + 1, this.numY - 1);
//
//	//	var sx = 1.0 - tx;
//	//	var sy = 1.0 - ty;
//
//	//	var val = sx * sy * f[x0 * n + y0] +
//	//		tx * sy * f[x1 * n + y0] +
//	//		tx * ty * f[x1 * n + y1] +
//	//		sx * ty * f[x0 * n + y1];
//
//	//	return val;
//	//}
//
//	//avgU(i, j) {
//	//	var n = this.numY;
//	//	var u = (this.u[i * n + j - 1] + this.u[i * n + j] +
//	//		this.u[(i + 1) * n + j - 1] + this.u[(i + 1) * n + j]) * 0.25;
//	//	return u;
//
//	//}
//
//	//avgV(i, j) {
//	//	var n = this.numY;
//	//	var v = (this.v[(i - 1) * n + j] + this.v[i * n + j] +
//	//		this.v[(i - 1) * n + j + 1] + this.v[i * n + j + 1]) * 0.25;
//	//	return v;
//	//}
//
//	//advectVel(dt) {
//
//	//	this.newU.set(this.u);
//	//	this.newV.set(this.v);
//
//	//	var n = this.numY;
//	//	var h = this.h;
//	//	var h2 = 0.5 * h;
//
//	//	for (var i = 1; i < this.numX; i++) {
//	//		for (var j = 1; j < this.numY; j++) {
//
//	//			cnt++;
//
//	//			// u component
//	//			if (this.s[i * n + j] != 0.0 && this.s[(i - 1) * n + j] != 0.0 && j < this.numY - 1) {
//	//				var x = i * h;
//	//				var y = j * h + h2;
//	//				var u = this.u[i * n + j];
//	//				var v = this.avgV(i, j);
//	//				//						var v = this.sampleField(x,y, V_FIELD);
//	//				x = x - dt * u;
//	//				y = y - dt * v;
//	//				u = this.sampleField(x, y, U_FIELD);
//	//				this.newU[i * n + j] = u;
//	//			}
//	//			// v component
//	//			if (this.s[i * n + j] != 0.0 && this.s[i * n + j - 1] != 0.0 && i < this.numX - 1) {
//	//				var x = i * h + h2;
//	//				var y = j * h;
//	//				var u = this.avgU(i, j);
//	//				//						var u = this.sampleField(x,y, U_FIELD);
//	//				var v = this.v[i * n + j];
//	//				x = x - dt * u;
//	//				y = y - dt * v;
//	//				v = this.sampleField(x, y, V_FIELD);
//	//				this.newV[i * n + j] = v;
//	//			}
//	//		}
//	//	}
//
//	//	this.u.set(this.newU);
//	//	this.v.set(this.newV);
//	//}
//
//	//advectSmoke(dt) {
//
//	//	this.newM.set(this.m);
//
//	//	var n = this.numY;
//	//	var h = this.h;
//	//	var h2 = 0.5 * h;
//
//	//	for (var i = 1; i < this.numX - 1; i++) {
//	//		for (var j = 1; j < this.numY - 1; j++) {
//
//	//			if (this.s[i * n + j] != 0.0) {
//	//				var u = (this.u[i * n + j] + this.u[(i + 1) * n + j]) * 0.5;
//	//				var v = (this.v[i * n + j] + this.v[i * n + j + 1]) * 0.5;
//	//				var x = i * h + h2 - dt * u;
//	//				var y = j * h + h2 - dt * v;
//
//	//				this.newM[i * n + j] = this.sampleField(x, y, S_FIELD);
//	//			}
//	//		}
//	//	}
//	//	this.m.set(this.newM);
//	//}
//
//	// ----------------- end of simulator ------------------------------


//	void simulate(float dt, float gravity, int numIters) {
//
//		integrate(dt, gravity);
//
//		for (auto& v : p) {
//			v = 0.0f;
//		}
//		solveIncompressibility(numIters, dt);
//
//		/*extrapolate();
//		advectVel(dt);
//		advectSmoke(dt);*/
//	}
//};

// https://www.youtube.com/watch?v=iKAVRgIrUOU
// Eulerian means grid based.
// Lagrangian methods are not grid based for example particles.

// Assuemes that the fluid is incompressible.
// Zero viscosity
struct EulerianFluid {
	Fluid fluid;
	EulerianFluid(Gfx& gfx);
	auto update(Gfx& gfx, Renderer& renderer) -> void;
	// Collocated grid vs staggered grid.
	// For fluid simulations a staggered grid is better.
	// +2 for walls
	static constexpr Vec2T<i64> GRID_SIZE{ 96 + 2, 50 + 2 };
	Vec2 velocities[GRID_SIZE.x][GRID_SIZE.y];
	decltype(velocities) oldVelocities;
	float preassure[GRID_SIZE.x][GRID_SIZE.y]{};
	float smoke[GRID_SIZE.x][GRID_SIZE.y]{};
	decltype(smoke) oldSmoke;
	std::bitset<GRID_SIZE.x * GRID_SIZE.y> isWallBitset;
	auto isWall(i64 x, i64 y) -> bool;
	auto setIsWall(i64 x, i64 y, bool value) -> void;
	static constexpr float SPACE_BETWEEN_CELLS = 0.02f;
	DynamicTexture texture;
};

template<typename T>
auto Fluid::at(std::vector<T>& vec, i64 x, i64 y) -> T& {
	return vec[x * numX + y];
}
