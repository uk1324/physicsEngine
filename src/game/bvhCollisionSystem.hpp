#pragma once

#include <utils/int.hpp>
#include <math/aabb.hpp>
#include <game/ent.hpp>
#include <game/collisionSystem.hpp>

#include <vector>
#include <unordered_set>

struct CollisionSystemProfile {
	float updateTime;
	float collisionCheckTime;
	i32 collisionsChecked;
};

class BvhCollisionSystem {
public:
	BvhCollisionSystem();

private:
	std::vector<u32> nodesToRemove;
public:
	auto update() -> void;
	auto reset() -> void;
	auto detectCollisions(CollisionMap& collisions) -> void;

	auto raycast(Vec2 start, Vec2 end) const -> std::optional<RaycastResult>;

	//std::unordered_set<BodyPair> collisionsToIgnore;

private:
	auto raycastHelper(u32 nodeIndex, Vec2 start, Vec2 end) const -> std::optional<RaycastResult>;
	auto clearCrossedFlag(u32 nodeIndex) -> void;
	auto collide(CollisionMap& collisions, u32 nodeA, u32 nodeB) -> void;

	static auto addMarginToAabb(const Aabb& aabb) -> Aabb;

	CollisionMap newCollisions;

	struct Node {
		u32 parent;
		u32 children[2];
		BodyId body;
		Aabb aabb;
		bool childrenCrossChecked;
		auto isLeaf() const -> bool { return children[0] == NULL_NODE; }
	};

	auto insert(BodyId bodyId) -> void;
	auto insertHelper(u32 parentNode, u32 nodeToInsert) -> u32;

	auto removeLeafNode(u32 nodeToRemove) -> void;

	u32 rootNode;
	std::vector<u32> leafNodes;

	static constexpr auto NULL_NODE = std::numeric_limits<u32>::max();
	auto allocateNode() -> u32;
	auto freeNode(u32 index) -> void;
	auto node(u32 index) -> Node&;
	auto node(u32 index) const -> const Node&;
	auto debugPrint(u32 rootNodeIndex) -> void;
	auto debugPrintHelper(u32 rootNodeIndex, i32 depth) -> void;
	auto debugDrawAabbs(u32 rootNodeIndex, i32 depth = 0) -> void;

	// Handles can get invalidated on a allocate call. Could create a class with an overloaded opeartor->, but it would need to store the instance of the collision system inside it.
	std::vector<Node> nodes;
	std::vector<u32> freeNodes;
};