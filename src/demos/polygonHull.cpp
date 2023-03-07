#include <demos/polygonHull.hpp>

#include <engine/renderer.hpp>
#include <utils/dbg.hpp>
#include <algorithm>
namespace {
	thread_local std::vector<Vec2> computeHullResult;
}
std::vector<int> test;
//auto computeHull(Span<const Vec2> points, std::vector<Vec2>& result = computeHullResult) -> const std::vector<Vec2>& {
//	result.clear();
//	test.clear();
//
//	std::vector<LineSegment> segments;
//	Vec2 current = points[0];
//	result.push_back(points[0]);
//	const auto start = 0;
//	int next = 1;
//	auto direction = 1;
//	std::optional<int> directionIfComplete;
//	std::optional<int> currentIndex = 0;
//	bool justChangedDirection = false;
//	std::optional<int> ignoredLine0;
//	std::optional<int> ignoredLine1;
//	do {
//		const auto sizeBefore = result.size();
//		/*if (next == 1)
//			_CrtDbgBreak();*/
//		const auto line = LineSegment{ current, points[next] };
//		int previous = points.size() - 1;
//		std::optional<Vec2> closestIntersection;
//		int closestIntersectioNext;
//		float closestIntersectionDistance = std::numeric_limits<float>::infinity();
//
//		if (direction == 1) {
//			if (ignoredLine1.has_value()) {
//				test.push_back(*ignoredLine1);
//			} else {
//				test.push_back(123123);
//			}
//		} else {
//			if (ignoredLine0.has_value()) {
//				test.push_back(*ignoredLine0);
//			} else {
//				test.push_back(123123);
//			}
//		}
//
//		for (int i = 0; i < points.size(); previous = i, i++) {
//
//			int n = next + direction;
//			/*if (justChangedDirection) {
//				n += direction;
//			} else {
//				n -= direction;
//			}*/
//			if (n < 0) {
//				n = points.size() - 1;
//			}
//			if (n >= points.size()) {
//				n = 0;
//			}
//			/*if (i == next || previous == next || i == currentIndex || previous == currentIndex || i == n || previous == n) {
//				continue;
//			}*/
//			if ((i == next && previous == n) || (i == n && previous == next)) {
//				continue;
//			}
//			
//
//			//if (direction == 1) {
//			//	if (i == ignoredLine1 || previous == ignoredLine1)
//			//		continue;
//			//} else {
//			//	if (i == ignoredLine0 || previous == ignoredLine0)
//			//		continue;
//			//}
//			/*if (currentIndex.has_value() && currentIndex.value() == previous) {
//				_CrtDbgBreak();
//			}
//			if (currentIndex == 2) {
//				_CrtDbgBreak();
//			}*/
//			const auto l = LineSegment{ points[previous], points[i] };
//			if (const auto intersection = l.intersection(line)) {
//				Debug::drawPoint(*intersection, Vec3::RED);
//				const auto d = distance(current, *intersection);
//				if (d < closestIntersectionDistance) {
//					closestIntersectionDistance = d;
//					closestIntersection = *intersection;
//					if (direction == 1) {
//						closestIntersectioNext = previous;
//					} else {
//						closestIntersectioNext = i;
//					}
//				}
//			}
//		}
//		if (closestIntersection.has_value()) {
//			current = *closestIntersection;
//			/*ignoredLine0 = currentIndex;
//			ignoredLine1 = next;*/
//			ignoredLine0 = currentIndex;
//			ignoredLine1 = next;
//			currentIndex = std::nullopt;
//			//next = previous;
//			next = closestIntersectioNext;
//			/*if (direction == 1) {
//				next = previous;
//			} else {
//				next = i;
//			}*/
//			//directionIfComplete= -direction;
//			direction = -direction;
//			justChangedDirection = true;
//			result.push_back(current);
//		} else {
//			justChangedDirection = false;
//
//			/*ignoredLine0 = currentIndex;
//			ignoredLine1 = next;*/
//			ignoredLine0 = next;
//			ignoredLine1 = currentIndex;
//			current = points[next];
//			currentIndex = next;
//			next = next + direction;
//			if (next >= points.size()) {
//				next = 0;
//			}
//			if (next < 0) {
//				next = points.size() - 1;
//			}
//			/*if (directionIfComplete.has_value()) {
//				direction = *directionIfComplete;
//				directionIfComplete = std::nullopt;
//			}*/
//			result.push_back(current);
//		}
//
//		if (result.size() == sizeBefore || result.size() > points.size() * 5) {
//			break;
//		}
//	} while (distance(current, points[0]) > 0.01f);
//
//	//for (int i = 0; i < 3 && i < points.size(); i++) {
//	//	result.push_back(points[i]);
//	//}
//
//	//for (int i = 4; i < points.size(); i++) {
//	//	const auto& toInsert = points[i];
//	//	if (!isPointInPolygon(result, toInsert)) {
//	//		int closest = 0;
//	//		int secondClosest;
//	//		float closestDistance = std::numeric_limits<float>::infinity();
//	//		for (int j = 1; j < result.size(); j++) {
//	//			const auto d = distance(result[j], toInsert);
//	//			if (d < closestDistance) {
//	//				closestDistance = d;
//	//				secondClosest = closest;
//	//				closest = j;
//	//			}
//	//		}
//	//		//result.push_back(points[i]);
//	//		result.insert(result.begin() + closest, toInsert);
//	//		/*std::erase_if(result, [&](const Vec2& p) {
//	//			return isPointInPolygon(result, p);
//	//		});*/
//	//	}
//	//}
//
//	return result;
//}
#include <engine/input.hpp>
#include <math/polygon.hpp>
//
//auto computeHull(Span<const Vec2> pointsA, std::vector<Vec2>& result = computeHullResult) -> const std::vector<Vec2>& {
//	result.clear();
//	test.clear();
//
//	std::vector<Vec2> points{ pointsA.begin(), pointsA.end() };
//	if (simplePolygonIsClockwise(points)) {
//		//std::reverse(points.begin(), points.end());
//	}
//	std::vector<LineSegment> segments;
//	Vec2 current = points[0];
//	result.push_back(points[0]);
//	int next = 1;
//	auto direction = 1;
//	std::optional<int> currentIndex = 0;
//
//	std::optional<int> lineToIgnore00;
//	int lineToIgnore01;
//	std::optional<int> lineToIgnore10 = 0;
//	int lineToIgnore11 = 1;
//	do {
//		const auto sizeBefore = result.size();
//		const auto line = LineSegment{ current, points[next] };
//		int previous = points.size() - 1;
//
//		std::optional<Vec2> closestIntersection;
//		int closestIntersectionLineStart;
//		int closestIntersectionLineEnd;
//		float closestIntersectionDistance = std::numeric_limits<float>::infinity();
//		for (int i = 0; i < points.size(); previous = i, i++) {
//			if (i == next || previous == next)
//				continue;
//
//			if (i == currentIndex || previous == currentIndex)
//				continue;
//
//			
//			if (lineToIgnore00.has_value()) {
//				int lineToIgnore01 = *lineToIgnore00 - direction;
//				if (lineToIgnore01 < 0) {
//					lineToIgnore01 = points.size() - 1;
//				}
//				if (lineToIgnore01 >= points.size()) {
//					lineToIgnore01 = 0;
//				}
//				if ((lineToIgnore00 == previous && lineToIgnore01 == i) || (lineToIgnore01 == previous && lineToIgnore00 == i)) {
//					continue;
//				}
//			}
//			if (lineToIgnore10.has_value()) {
//				if ((lineToIgnore10 == previous && lineToIgnore11 == i) || (lineToIgnore11 == previous && lineToIgnore10 == i)) {
//					continue;
//				}
//			}
//
//			const auto l = LineSegment{ points[previous], points[i] };
//			if (const auto intersection = l.intersection(line)) {
//				//Debug::drawPoint(*intersection, Vec3::RED);
//				const auto d = distance(current, *intersection);
//				if (d < closestIntersectionDistance) {
//					// !!!! COULDN'T i JUST CHECK IF IT IS AN EAR IN HERE
//					// no probably because I don't know what point creates a triangle with it
//					closestIntersectionDistance = d;
//					closestIntersection = *intersection;
//					closestIntersectionLineStart = previous;
//					closestIntersectionLineEnd = i;
//					if (direction == 1) {
//						closestIntersectionLineStart = previous;
//					} else {
//						closestIntersectionLineStart = i;
//					}
//				}
//			}
//		}
//		if (closestIntersection.has_value()) {
//			if (currentIndex.has_value()) {
//				lineToIgnore10 = currentIndex;
//			} else {
//				// Keep it the same.
//			}
//			lineToIgnore11 = next;
//
//			int previousJ = points.size() - 1;
//			LineSegment ll{ points[closestIntersectionLineStart], points[closestIntersectionLineEnd] };
//			float closestPlusDist = std::numeric_limits<float>::infinity();
//			float closestMinusDist = std::numeric_limits<float>::infinity();
//			std::optional<Vec2> closestPlus;
//			std::optional<Vec2> closestMinus;
//			for (int j = 0; j < points.size(); previousJ = j, j++) {
//				int start = previous;
//				int end = j;
//				if (start == closestIntersectionLineStart || end == closestIntersectionLineEnd
//					|| start == closestIntersectionLineEnd || end == closestIntersectionLineStart) {
//					continue;
//				}
//
//				if ((start == lineToIgnore10 && end == lineToIgnore11) || (end == lineToIgnore10 && start == lineToIgnore11)) {
//					continue;
//				}
//
//				const auto interse = ll.intersection(LineSegment{ points[start], points[end] });
//				if (!interse.has_value()) {
//					continue;
//				}
//
//				/*if (auto d = signedDistance(line.line, *interse) * direction > 0.0f) {*/
//				if (auto d = signedDistance(line.line, *interse) > 0.0f) {
//					auto da = std::abs(d);
//					if (da < closestMinusDist) {
//						closestMinusDist = da;
//						closestMinus = *interse;
//					}
//				} else {
//					auto da = std::abs(d);
//					if (da < closestPlusDist) {
//						closestPlusDist = da;
//						closestPlus = *interse;
//					}
//				}
//			}
//
//			auto isEar = [&](Vec2 a, Vec2 b, Vec2 c) -> bool {
//				auto r = Triangle{ a, b, c }.isClockwise();
//				if (direction == 1) {
//					r = !r;
//				}
//				return r;
//			};
//
//			if (closestPlus.has_value() && closestMinus.has_value()) {
//				if (isEar(current, *closestPlus, *closestIntersection)) {
//					std::swap(closestIntersectionLineStart, closestIntersectionLineEnd);
//					direction = -direction;
//				} else {
//					//ASSERT(!isEar(current, *closestIntersection, *closestMinus));
//					//direction = -direction;
//				}
//			} else if (closestPlus.has_value()) {
//				if (isEar(current, *closestPlus, *closestIntersection)) {
//					std::swap(closestIntersectionLineStart, closestIntersectionLineEnd);
//					direction = -direction;
//				} else {
//					//direction = -direction;
//				}
//			} else if (closestMinus.has_value()) {
//				if (!isEar(current, *closestMinus, *closestIntersection)) {
//					std::swap(closestIntersectionLineStart, closestIntersectionLineEnd);
//					direction = -direction;
//				} else {
//					//direction = -direction;
//				}
//			} else {
//				/*if (direction == 1) {
//					next = closestIntersectionLineStart;
//				} else {
//					next = closestIntersectionLineEnd;
//				}*/
//				if (direction == -1) {
//					std::swap(closestIntersectionLineStart, closestIntersectionLineEnd);
//				}
//			}
//
//
//			
//			next = closestIntersectionLineStart;
//			lineToIgnore00 = next;
//
//			current = *closestIntersection;
//			currentIndex = std::nullopt;
//
//
//			direction = -direction;
//			result.push_back(current);
//		} else {
//			lineToIgnore10 = currentIndex;
//			lineToIgnore11 = next;
//			lineToIgnore00 = std::nullopt;
//			current = points[next];
//			currentIndex = next;
//			next = next + direction;
//			if (next >= points.size()) {
//				next = 0;
//			}
//			if (next < 0) {
//				next = points.size() - 1;
//			}
//			result.push_back(current);
//		}
//
//		if (result.size() == sizeBefore || result.size() > points.size() * 5) {
//			break;
//		}
//	} while (distance(current, points[0]) > 0.01f);
//
//	return result;
//}

//struct Line {
	//	Line(int start, int end) {
	//		if (start < end) {
	//			this->start = start;
	//			this->end = end;
	//		} else {
	//			this->start = end;
	//			this->end = start;
	//		}
	//	}

	//	auto operator==(const Line& other) const -> bool = default;

	//	int start, end;
	//};

	//struct Intersection {
	//	Vec2 pos;
	//	Line a;
	//	Line b;
	//};

	//std::vector<Intersection> intersections;
	//int previousI = points.size() - 1;
	//for (int i = 0; i < points.size(); previousI = i, i++) {
	//	int previousJ = points.size() - 1;
	//	for (int j = 0; j < points.size(); previousJ = j, j++) {
	//		if (i == j || i == previousJ || j == previousI)
	//			continue;

	//		if (auto intersection = LineSegment{ points[i], points[previousI] }.intersection(LineSegment{ points[j], points[previousJ] })) {
	//			Debug::drawPoint(*intersection);
	//			intersections.push_back({ *intersection, Line{ i, previousI }, Line{ j, previousJ } });
	//		}
	//	}

// TODO: !!!! Make it possible to set a flag that makes it so a call to Debug::draw actually draws and presents the new image after the call is completed.  This would be really useful for debugging. Another option would be to make the function tepmplated implement some inspector / vistor thing that allows the caller to pass a visitor the data about the algorithm. The best option would be to do something like valve does and have a separate program that can read the memory of the game and visualize everything in real time. Or maybe just add some #ifdefs that just enable the debugging code instead of using a template.

/*std::vector<std::vector<Vec2>> polygons;
	int k = 0;
	for (const auto [intersection, lineA, lineB] : intersections) {
		int previousI = points.size() - 1;
		struct Crossing {
			Vec2 v;
			Line l;
			bool isIntersection;
		};
		std::vector<Crossing> crossings;
		Debug::drawLine(intersection - Vec2{ 0.0f, 10.0f }, intersection + Vec2{ 0.0f, 10.0f }, Vec3::RED);
		for (int i = 0; i < points.size(); previousI = i, i++) {
			bool isIntersection = true;
			if (Line{ i, previousI } == lineA || Line{ i, previousI } == lineB) {
				continue;
			}

			auto line = LineSegment{ intersection - Vec2{ 0.0f, 10.0f }, intersection + Vec2{ 0.0f, 10.0f } };
			if (auto intersection = line.intersection(LineSegment{ points[i], points[previousI] })) {
				Debug::drawPoint(*intersection, Vec3::RED);
				crossings.push_back({ *intersection, Line{ i, previousI } });
			}
		}

		std:sort(crossings.begin(), crossings.end(), [](const std::pair<Vec2, Line>& a, const std::pair<Vec2, Line>& b) {
			return (a.first.y < b.first.y);
		});

		for (int i = 0; i < crossings.size(); i++) {
			if (i % 2 == 0) {
				Debug::drawPoint(crossings[i].first, Vec3::RED);
			} else {
				Debug::drawPoint(crossings[i].first, Vec3::GREEN);
			}
		}

		if (k == 0) {

		}
		k++;
	}*/

auto PolygonHullDemo::update() -> void {
	Camera camera;

	/*Vec2 points[] = {
		Vec2{ -1.0f, -1.0f },
		Vec2{ 1.0f, 1.0f },
		Vec2{ -1.0f, 1.0f },
		Vec2{ 1.0f, -1.0f }
	};*/
	/*std::vector<Vec2> points{
		Vec2{ -1.0f, -1.0f },
		Vec2{ 1.0f, 1.0f },
		Vec2{ -1.0f, 1.0f },
		Vec2{ 1.0f, -1.0f }
	};*/
	//static std::vector<Vec2> points;
	//if (Input::isMouseButtonDown(MouseButton::LEFT)) {
	//	points.push_back(camera.cursorPos());
	//}
	///*for (auto& p : points) {
	//	p = p / 3.0f;
	//}*/
	//auto result = area(points);
	//chk(chkek) {
	//	Debug::drawLines(points);
	//	Debug::drawLines(result, Vec3::RED);
	//} else {
	//	Debug::drawLines(points);
	//}
	//intin(max, 0);
	//for (int i = 0; i < max; i++) {
	//	Debug::drawText(result[i], i);
	//}
	//chk(dis) {
	//	for (int i = 0; i < points.size(); i++) {
	//		Debug::drawText(points[i], i);
	//	}
	//}

	/*chk(display) {
	} else {
	}*/
	//for (const auto a : test) {
	//	ImGui::Text("%d", a);
	//}
	//intin(max, 0);
	//chk(displayHull) {
	//	if (points.size() >= 3) {
	//		auto hull = computeHull(points);
	//		Debug::drawLines(hull);
	//		for (int i = 0; i < max; i++) {
	//			Debug::drawPoint(hull[i]);
	//		}
	//		if (max > 0)
	//			Debug::drawPoint(hull[max - 1], Vec3::RED);
	//		Debug::drawPoint(hull[max], Vec3::GREEN);
	//	}
	//} else {
	//	Debug::drawLines(points);
	//}
	//for (int i = 0; i < points.size(); i++) {
	//	Debug::drawText(points[i], i);
	//}


	Renderer::update(camera);
}


//auto computeHull(Span<const Vec2> points, std::vector<Vec2>& result = computeHullResult) -> const std::vector<Vec2>& {
//	result.clear();
//
//
//	std::vector<LineSegment> segments;
//	Vec2 current = points[0];
//	result.push_back(points[0]);
//	const auto start = 0;
//	int next = 1;
//	auto direction = 1;
//	std::optional<int> directionIfComplete;
//	std::optional<int> currentIndex = 0;
//	bool justChangedDirection = false;
//	std::optional<int> ignoredLine0;
//	std::optional<int> ignoredLine1;
//	std::optional<int> ignoredLine2;
//	std::optional<int> ignoredLine3;
//	do {
//		const auto sizeBefore = result.size();
//		const auto line = LineSegment{ current, points[next] };
//		int previous = points.size() - 1;
//		bool intersected = false;
//		for (int i = 0; i < points.size(); previous = i, i++) {
//			int n = next;
//			//if (justChangedDirection) {
//			//	n += direction;
//			//} else {
//			//	n -= direction;
//			//}
//			//if (n < 0) {
//			//	n = points.size() - 1;
//			//}
//			//if (n >= points.size()) {
//			//	n = 0;
//			//}
//			if (i == next || previous == next || i == currentIndex || previous == currentIndex) {
//				continue;
//			}
//			if (i == ignoredLine0 || previous == ignoredLine0)
//				continue;
//
//			if (i == ignoredLine1 || previous == ignoredLine1)
//				continue;
//			/*if (currentIndex.has_value() && currentIndex.value() == previous) {
//				_CrtDbgBreak();
//			}
//			if (currentIndex == 2) {
//				_CrtDbgBreak();
//			}*/
//			const auto l = LineSegment{ points[previous], points[i] };
//			if (const auto intersection = l.intersection(line)) {
//				current = *intersection;
//				ignoredLine0 = currentIndex;
//				ignoredLine1 = next;
//				currentIndex = std::nullopt;
//				Debug::drawPoint(current, Vec3::BLUE);
//				next = previous;
//				if (direction == 1) {
//					next = previous;
//				} else {
//					next = i;
//				}
//				//directionIfComplete= -direction;
//				direction = -direction;
//				justChangedDirection = true;
//				intersected = true;
//				result.push_back(current);
//				break;
//			}
//		}
//		if (!intersected) {
//			justChangedDirection = false;
//
//			ignoredLine0 = currentIndex;
//			ignoredLine1 = next;
//			current = points[next];
//			currentIndex = next;
//			next = next + direction;
//			if (next >= points.size()) {
//				next = 0;
//			}
//			if (next < 0) {
//				next = points.size() - 1;
//			}
//			/*if (directionIfComplete.has_value()) {
//				direction = *directionIfComplete;
//				directionIfComplete = std::nullopt;
//			}*/
//			result.push_back(current);
//		}
//
//		if (result.size() == sizeBefore || result.size() > points.size() * 5) {
//			break;
//		}
//	} while (distance(current, points[0]) > 0.01f);
//
//	//for (int i = 0; i < 3 && i < points.size(); i++) {
//	//	result.push_back(points[i]);
//	//}
//
//	//for (int i = 4; i < points.size(); i++) {
//	//	const auto& toInsert = points[i];
//	//	if (!isPointInPolygon(result, toInsert)) {
//	//		int closest = 0;
//	//		int secondClosest;
//	//		float closestDistance = std::numeric_limits<float>::infinity();
//	//		for (int j = 1; j < result.size(); j++) {
//	//			const auto d = distance(result[j], toInsert);
//	//			if (d < closestDistance) {
//	//				closestDistance = d;
//	//				secondClosest = closest;
//	//				closest = j;
//	//			}
//	//		}
//	//		//result.push_back(points[i]);
//	//		result.insert(result.begin() + closest, toInsert);
//	//		/*std::erase_if(result, [&](const Vec2& p) {
//	//			return isPointInPolygon(result, p);
//	//		});*/
//	//	}
//	//}
//
//	return result;
//}













//auto computeHull(Span<const Vec2> points, std::vector<Vec2>& result = computeHullResult) -> const std::vector<Vec2>& {
//	result.clear();
//	test.clear();
//
//	std::vector<LineSegment> segments;
//	Vec2 current = points[0];
//	result.push_back(points[0]);
//	const auto start = 0;
//	int next = 1;
//	auto direction = 1;
//	std::optional<int> directionIfComplete;
//	std::optional<int> currentIndex = 0;
//	bool justChangedDirection = false;
//	std::optional<int> ignoredLine0;
//	std::optional<int> ignoredLine1;
//	do {
//		const auto sizeBefore = result.size();
//		/*if (next == 1)
//			_CrtDbgBreak();*/
//		const auto line = LineSegment{ current, points[next] };
//		int previous = points.size() - 1;
//		std::optional<Vec2> closestIntersection;
//		int closestIntersectioNext;
//		float closestIntersectionDistance = std::numeric_limits<float>::infinity();
//
//		if (direction == 1) {
//			if (ignoredLine1.has_value()) {
//				test.push_back(*ignoredLine1);
//			} else {
//				test.push_back(123123);
//			}
//		} else {
//			if (ignoredLine0.has_value()) {
//				test.push_back(*ignoredLine0);
//			} else {
//				test.push_back(123123);
//			}
//		}
//
//		for (int i = 0; i < points.size(); previous = i, i++) {
//
//			int n = next;
//			//if (justChangedDirection) {
//			//	n += direction;
//			//} else {
//			//	n -= direction;
//			//}
//			//if (n < 0) {
//			//	n = points.size() - 1;
//			//}
//			//if (n >= points.size()) {
//			//	n = 0;
//			//}
//			if (i == next || previous == next || i == currentIndex || previous == currentIndex) {
//				continue;
//			}
//
//
//			if (direction == 1) {
//				if (i == ignoredLine1 || previous == ignoredLine1)
//					continue;
//			} else {
//				if (i == ignoredLine0 || previous == ignoredLine0)
//					continue;
//			}
//			/*if (currentIndex.has_value() && currentIndex.value() == previous) {
//				_CrtDbgBreak();
//			}
//			if (currentIndex == 2) {
//				_CrtDbgBreak();
//			}*/
//			const auto l = LineSegment{ points[previous], points[i] };
//			if (const auto intersection = l.intersection(line)) {
//				Debug::drawPoint(*intersection, Vec3::RED);
//				const auto d = distance(current, *intersection);
//				if (d < closestIntersectionDistance) {
//					closestIntersectionDistance = d;
//					closestIntersection = *intersection;
//					if (direction == 1) {
//						closestIntersectioNext = previous;
//					} else {
//						closestIntersectioNext = i;
//					}
//				}
//			}
//		}
//		if (closestIntersection.has_value()) {
//			current = *closestIntersection;
//			/*ignoredLine0 = currentIndex;
//			ignoredLine1 = next;*/
//			ignoredLine0 = currentIndex;
//			ignoredLine1 = next;
//			currentIndex = std::nullopt;
//			//next = previous;
//			next = closestIntersectioNext;
//			/*if (direction == 1) {
//				next = previous;
//			} else {
//				next = i;
//			}*/
//			//directionIfComplete= -direction;
//			direction = -direction;
//			justChangedDirection = true;
//			result.push_back(current);
//		} else {
//			justChangedDirection = false;
//
//			/*ignoredLine0 = currentIndex;
//			ignoredLine1 = next;*/
//			ignoredLine0 = next;
//			ignoredLine1 = currentIndex;
//			current = points[next];
//			currentIndex = next;
//			next = next + direction;
//			if (next >= points.size()) {
//				next = 0;
//			}
//			if (next < 0) {
//				next = points.size() - 1;
//			}
//			/*if (directionIfComplete.has_value()) {
//				direction = *directionIfComplete;
//				directionIfComplete = std::nullopt;
//			}*/
//			result.push_back(current);
//		}
//
//		if (result.size() == sizeBefore || result.size() > points.size() * 5) {
//			break;
//		}
//	} while (distance(current, points[0]) > 0.01f);
//
//	//for (int i = 0; i < 3 && i < points.size(); i++) {
//	//	result.push_back(points[i]);
//	//}
//
//	//for (int i = 4; i < points.size(); i++) {
//	//	const auto& toInsert = points[i];
//	//	if (!isPointInPolygon(result, toInsert)) {
//	//		int closest = 0;
//	//		int secondClosest;
//	//		float closestDistance = std::numeric_limits<float>::infinity();
//	//		for (int j = 1; j < result.size(); j++) {
//	//			const auto d = distance(result[j], toInsert);
//	//			if (d < closestDistance) {
//	//				closestDistance = d;
//	//				secondClosest = closest;
//	//				closest = j;
//	//			}
//	//		}
//	//		//result.push_back(points[i]);
//	//		result.insert(result.begin() + closest, toInsert);
//	//		/*std::erase_if(result, [&](const Vec2& p) {
//	//			return isPointInPolygon(result, p);
//	//		});*/
//	//	}
//	//}
//
//	return result;
//}