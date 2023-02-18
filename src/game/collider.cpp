#include <game/collider.hpp>

auto BoxCollider::aabb(const Transform& transform) const -> Aabb {
	const auto corners = getCorners(transform);
	return Aabb::fromPoints(Span{ corners.data(), corners.size() });
}

auto BoxCollider::getCorners(const Transform& transform) const->std::array<Vec2, 4> {
	const auto normals = transform.rot.toMatrix();
	const auto cornerDir = normals.x() * (size.x / 2.0f) + normals.y() * (size.y / 2.0f);
	return { 
		transform.pos + cornerDir, 
		transform.pos + cornerDir - normals.x() * size.x, 
		transform.pos - cornerDir, 
		transform.pos - cornerDir + normals.x() * size.x 
	};
}

auto BoxCollider::getEdges(const Transform& transform) const->std::array<LineSegment, 4> {
	// @Performance Could do this by translating line segments.
	/*const auto normals = Mat2::rotate(orientation);
	LineSegment{ Line{ normals.x(),  } }*/
	const auto corners = getCorners(transform);
	return {
		LineSegment{ corners[0], corners[1] },
		LineSegment{ corners[1], corners[2] },
		LineSegment{ corners[2], corners[3] },
		LineSegment{ corners[3], corners[0] }
	};
}

auto CircleCollider::massInfo(float density) const -> MassInfo {
	const auto mass = TAU<float> * pow(radius, 2.0f) * density;
	return MassInfo{
		.mass = mass,
		.rotationalInertia = mass * pow(radius, 2.0f) / 2.0f
	};
}

auto CircleCollider::aabb(const Transform& transform) const -> Aabb {
	return Aabb{ transform.pos + Vec2{ -radius }, transform.pos + Vec2{ radius } };
}
