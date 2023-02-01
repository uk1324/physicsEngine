#include <game/convexPolygonCollider.hpp>
#include <game/collider.hpp>

auto ConvexPolygon::regular(i32 vertCount, float radius) -> ConvexPolygon {
    ConvexPolygon polygon;
    const auto angleStep = TAU<float> / vertCount;
    for (i32 i = 0; i < vertCount; i++) {
        polygon.verts.push_back(Vec2::oriented(angleStep / 2.0f + angleStep * i) * radius);
    }
    polygon.calculateNormals();
    return polygon;
}

auto ConvexPolygon::calculateNormals() -> void {
    normals.clear();
    if (verts.size() < 3) {
        ASSERT_NOT_REACHED();
        return;
    }

    for (usize i = 0; i < verts.size() - 1; i++) {
        normals.push_back((verts[i + 1] - verts[i]).rotBy90deg().normalized());
    }
    normals.push_back((verts[0] - verts.back()).rotBy90deg().normalized());
}

auto ConvexPolygon::aabb(const Transform& transform) const -> Aabb {
    ASSERT(verts.size() >= 2);
    Aabb aabb{ Vec2{ std::numeric_limits<float>::infinity() }, Vec2{ -std::numeric_limits<float>::infinity() } };
    for (const auto& vert : verts) {
        aabb.min = aabb.min.min(vert);
        aabb.max = aabb.max.max(vert);
    }
    return aabb;
}

auto ConvexPolygon::massInfo(float density) const -> MassInfo {
    double area = 0.0;
    // Calculate value of shoelace formula
    int j = verts.size() - 1;
    for (int i = 0; i < verts.size(); i++)
    {
        area += (verts[j].x + verts[i].x) * (verts[j].y - verts[i].y);
        j = i;  // j is previous vertex to i
    }

    area = abs(area / 2.0);

    float mass = area * density;

    float inertia = 0.0f;
    if (verts.size() != 1) {
        float denom = 0.0f;
        float numer = 0.0f;
        for (int j = verts.size() - 1, i = 0; i < verts.size(); j = i, i++) {
            auto P0 = verts[j];
            auto P1 = verts[0];
            float a = (float)fabs(cross(P0, P1));
            float b = (dot(P1, P1) + dot(P1, P0) + dot(P0, P0));
            denom += (a * b);
            numer += a;
        }
        inertia = (mass / 6.0f) * (denom / numer);
        //return inertia;
    }

    return MassInfo{ mass, inertia };
}
