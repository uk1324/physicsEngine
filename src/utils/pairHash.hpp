#pragma once

#include <algorithm>

// https://stackoverflow.com/questions/20590656/error-for-hash-function-of-pair-of-ints
struct PairHash {
    template <typename T, typename U>
    auto operator()(const std::pair<T, U>& x) const -> size_t {
        return std::hash<T>{}(x.first) * std::hash<U>{}(x.second);
    }
};

// !!!! I was stupid the issue is that it used DistanceJoint instead of DistanceJointId
// doesn't work TODO: fix. This was supposted to be used for types like EntityArray<T>::Id because std::hash takes a different type as the template argument than the operator() argument. I even tried to replace the types by hand, but it keeps complaining that you can't do something like this
//struct Hasher {
//	auto operator()(const std::pair<DistanceJoint, BodyPair>& x) const -> size_t {
//		return  std::hash<EntityArray<DistanceJoint>>()(x.first) * std::hash<BodyPair>{}(x.second);
//		//return std::hash<EntityArray<DistanceJoint>>{}(x.first)* std::hash<BodyPair>{}(x.second);
//	}
//};
// Even though a similar thing is done in BodyPairHasher.

//template<typename A0, typename A1, typename B0, typename B1>
//struct PairHashAdvanced {
//    auto operator()(const std::pair<A0, B0>& x) const -> size_t {
//        return std::hash<A1>{}(x.first) * std::hash<B1>{}(x.second);
//    }
//};