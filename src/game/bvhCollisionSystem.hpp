#pragma once

#include <utils/int.hpp>
#include <math/aabb.hpp>
#include <game/collisionSystem.hpp>
#include <vector>

struct CollisionSystemProfile {
	float updateTime;
	float collisionCheckTime;
	i32 collisionsChecked;
};

class BvhCollisionSystem {
public:
	BvhCollisionSystem();

	auto update(const std::vector<Body*>& toAdd, const std::vector<Body*>& toRemove) -> void;
	auto detectCollisions(CollisionMap& collisions) -> void;

	auto raycast(Vec2 start, Vec2 end) const -> std::optional<RaycastResult>;

private:
	auto raycastHelper(u32 nodeIndex, Vec2 start, Vec2 end) const -> std::optional<RaycastResult>;
	auto clearCrossedFlag(u32 nodeIndex) -> void;
	auto collide(CollisionMap& collisions, u32 nodeA, u32 nodeB) -> void;

	CollisionMap newCollisions;

	static constexpr float FAT_AABB_MARGIN = 0.2f;

	struct Node {
		u32 parent;
		u32 children[2];
		Body* body;
		Aabb aabb;
		bool childrenCrossChecked;
		auto isLeaf() const -> bool { return children[0] == NULL_NODE; }
	};

	auto insert(Body& body) -> void;
	auto insertHelper(u32 parentNode, u32 nodeToInsert) -> u32;

	auto removeLeafNode(u32 nodeToRemove) -> void;

	u32 rootNode;
	std::unordered_map<const Body*, u32> leafNodes;

	static constexpr auto NULL_NODE = std::numeric_limits<u32>::max();
	auto allocateNode() -> u32;
	auto freeNode(u32 index) -> void;
	auto node(u32 index) -> Node&;
	auto node(u32 index) const -> const Node&;
	auto debugPrint(u32 rootNodeIndex) -> void;
	auto debugPrintHelper(u32 rootNodeIndex, i32 depth) -> void;
	auto debugDrawAabbs(u32 rootNodeIndex, i32 depth = 0) -> void;
	// For removing based on entity could store a map<Entity*, Node*>

	// Storing array of leaf nodes for removal.
	// Handles can get invalidated on a allocate call. Could create a class with an overloaded opeartor->, but it would need to store the instance of the class inside it.
	std::vector<Node> nodes;
	std::vector<u32> freeNodes;
};