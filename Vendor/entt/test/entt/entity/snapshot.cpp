#include <cstddef>
#include <map>
#include <memory>
#include <queue>
#include <tuple>
#include <type_traits>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/any.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/snapshot.hpp>
#include <entt/signal/sigh.hpp>
#include "../common/config.h"

struct empty {};

struct shadow {
    entt::entity target{entt::null};

    static void listener(entt::entity &elem, entt::registry &registry, const entt::entity entt) {
        elem = registry.get<shadow>(entt).target;
    }
};

TEST(BasicSnapshot, Constructors) {
    static_assert(!std::is_default_constructible_v<entt::basic_snapshot<entt::registry>>);
    static_assert(!std::is_copy_constructible_v<entt::basic_snapshot<entt::registry>>);
    static_assert(!std::is_copy_assignable_v<entt::basic_snapshot<entt::registry>>);
    static_assert(std::is_move_constructible_v<entt::basic_snapshot<entt::registry>>);
    static_assert(std::is_move_assignable_v<entt::basic_snapshot<entt::registry>>);

    entt::registry registry;
    entt::basic_snapshot snapshot{registry};
    entt::basic_snapshot other{std::move(snapshot)};

    ASSERT_NO_FATAL_FAILURE(snapshot = std::move(other));
}

TEST(BasicSnapshot, GetEntityType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot snapshot{registry};
    const auto &storage = registry.storage<entt::entity>();

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&elem) { data.emplace_back(std::forward<decltype(elem)>(elem)); };

    snapshot.get<entt::entity>(archive);

    ASSERT_EQ(data.size(), 2u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[1u]), storage.in_use());

    entt::entity entity[3u];

    registry.create(std::begin(entity), std::end(entity));
    registry.destroy(entity[1u]);

    data.clear();
    snapshot.get<entt::entity>(archive, "ignored"_hs);

    ASSERT_EQ(data.size(), 5u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[1u]), storage.in_use());

    ASSERT_NE(entt::any_cast<entt::entity>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[2u]), storage.data()[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[3u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[3u]), storage.data()[1u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[4u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[4u]), storage.data()[2u]);
}

TEST(BasicSnapshot, GetType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot snapshot{registry};
    const auto &storage = registry.storage<int>();

    entt::entity entity[3u];
    const int values[3u]{1, 2, 3};

    registry.create(std::begin(entity), std::end(entity));
    registry.insert<int>(std::begin(entity), std::end(entity), std::begin(values));
    registry.destroy(entity[1u]);

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&elem) { data.emplace_back(std::forward<decltype(elem)>(elem)); };

    snapshot.get<int>(archive, "other"_hs);

    ASSERT_EQ(data.size(), 1u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), 0u);

    data.clear();
    snapshot.get<int>(archive);

    ASSERT_EQ(data.size(), 5u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<entt::entity>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[1u]), entity[0u]);

    ASSERT_NE(entt::any_cast<int>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[2u]), values[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[3u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[3u]), entity[2u]);

    ASSERT_NE(entt::any_cast<int>(&data[4u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[4u]), values[2u]);
}

TEST(BasicSnapshot, GetEmptyType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot snapshot{registry};
    const auto &storage = registry.storage<empty>();

    entt::entity entity[3u];

    registry.create(std::begin(entity), std::end(entity));
    registry.insert<empty>(std::begin(entity), std::end(entity));
    registry.destroy(entity[1u]);

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&elem) { data.emplace_back(std::forward<decltype(elem)>(elem)); };

    snapshot.get<empty>(archive, "other"_hs);

    ASSERT_EQ(data.size(), 1u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), 0u);

    data.clear();
    snapshot.get<empty>(archive);

    ASSERT_EQ(data.size(), 3u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), storage.size());

    ASSERT_NE(entt::any_cast<entt::entity>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[1u]), entity[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[2u]), entity[2u]);
}

TEST(BasicSnapshot, GetTypeSparse) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot snapshot{registry};

    entt::entity entity[3u];
    const int values[3u]{1, 2, 3};

    registry.create(std::begin(entity), std::end(entity));
    registry.insert<int>(std::begin(entity), std::end(entity), std::begin(values));
    registry.destroy(entity[1u]);

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&elem) { data.emplace_back(std::forward<decltype(elem)>(elem)); };

    snapshot.get<int>(archive, std::begin(entity), std::end(entity), "other"_hs);

    ASSERT_EQ(data.size(), 1u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), 0u);

    data.clear();
    snapshot.get<int>(archive, std::begin(entity), std::end(entity));

    ASSERT_EQ(data.size(), 6u);

    ASSERT_NE(entt::any_cast<typename traits_type::entity_type>(&data[0u]), nullptr);
    ASSERT_EQ(entt::any_cast<typename traits_type::entity_type>(data[0u]), static_cast<typename traits_type::entity_type>(std::distance(std::begin(entity), std::end(entity))));

    ASSERT_NE(entt::any_cast<entt::entity>(&data[1u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[1u]), entity[0u]);

    ASSERT_NE(entt::any_cast<int>(&data[2u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[2u]), values[0u]);

    ASSERT_NE(entt::any_cast<entt::entity>(&data[3u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[3u]), static_cast<entt::entity>(entt::null));

    ASSERT_NE(entt::any_cast<entt::entity>(&data[4u]), nullptr);
    ASSERT_EQ(entt::any_cast<entt::entity>(data[4u]), entity[2u]);

    ASSERT_NE(entt::any_cast<int>(&data[5u]), nullptr);
    ASSERT_EQ(entt::any_cast<int>(data[5u]), values[2u]);
}

TEST(BasicSnapshotLoader, Constructors) {
    static_assert(!std::is_default_constructible_v<entt::basic_snapshot_loader<entt::registry>>);
    static_assert(!std::is_copy_constructible_v<entt::basic_snapshot_loader<entt::registry>>);
    static_assert(!std::is_copy_assignable_v<entt::basic_snapshot_loader<entt::registry>>);
    static_assert(std::is_move_constructible_v<entt::basic_snapshot_loader<entt::registry>>);
    static_assert(std::is_move_assignable_v<entt::basic_snapshot_loader<entt::registry>>);

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    entt::basic_snapshot_loader other{std::move(loader)};

    ASSERT_NO_FATAL_FAILURE(loader = std::move(other));
}

ENTT_DEBUG_TEST(BasicSnapshotLoaderDeathTest, Constructors) {
    entt::registry registry;
    registry.emplace<int>(registry.create());

    ASSERT_DEATH([[maybe_unused]] entt::basic_snapshot_loader loader{registry}, "");
}

TEST(BasicSnapshotLoader, GetEntityType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<entt::entity>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const entt::entity entity[3u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u), traits_type::construct(1u, 1u)};

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
    ASSERT_FALSE(registry.valid(entity[2u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));

    loader.get<entt::entity>(archive);

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
    ASSERT_FALSE(registry.valid(entity[2u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(storage.in_use(), 0u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);
    data.emplace_back(entity[2u]);

    loader.get<entt::entity>(archive, "ignored"_hs);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));
    ASSERT_FALSE(registry.valid(entity[2u]));

    ASSERT_EQ(storage.size(), 3u);
    ASSERT_EQ(storage.in_use(), 2u);

    ASSERT_EQ(storage[0u], entity[0u]);
    ASSERT_EQ(storage[1u], entity[1u]);
    ASSERT_EQ(storage[2u], entity[2u]);

    ASSERT_EQ(registry.create(), entity[2u]);
}

TEST(BasicSnapshotLoader, GetType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const entt::entity entity[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int values[2u]{1, 3};

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);
    data.emplace_back(values[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(values[0u]);

    data.emplace_back(entity[1u]);
    data.emplace_back(values[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(entity[0u]));
    ASSERT_TRUE(storage.contains(entity[1u]));
    ASSERT_EQ(storage.get(entity[0u]), values[0u]);
    ASSERT_EQ(storage.get(entity[1u]), values[1u]);
}

TEST(BasicSnapshotLoader, GetEmptyType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<empty>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const entt::entity entity[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);

    loader.get<empty>(archive, "other"_hs);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<empty>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);

    loader.get<empty>(archive);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(entity[0u]));
    ASSERT_TRUE(storage.contains(entity[1u]));
}

TEST(BasicSnapshotLoader, GetTypeSparse) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const entt::entity entity[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int values[2u]{1, 3};

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<entt::entity>(entt::null));
    data.emplace_back(entity[0u]);
    data.emplace_back(values[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));

    data.emplace_back(entity[0u]);
    data.emplace_back(values[0u]);

    data.emplace_back(static_cast<entt::entity>(entt::null));

    data.emplace_back(entity[1u]);
    data.emplace_back(values[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(entity[0u]));
    ASSERT_TRUE(storage.contains(entity[1u]));
    ASSERT_EQ(storage.get(entity[0u]), values[0u]);
    ASSERT_EQ(storage.get(entity[1u]), values[1u]);
}

TEST(BasicSnapshotLoader, GetTypeWithListener) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};
    entt::entity check{entt::null};

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const auto entity{traits_type::construct(1u, 1u)};
    const shadow value{entity};

    ASSERT_FALSE(registry.valid(entity));
    ASSERT_EQ(check, static_cast<entt::entity>(entt::null));

    registry.on_construct<shadow>().connect<&shadow::listener>(check);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity);
    data.emplace_back(value);

    loader.get<shadow>(archive);

    ASSERT_TRUE(registry.valid(entity));
    ASSERT_EQ(check, entity);
}

TEST(BasicSnapshotLoader, Orphans) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot_loader loader{registry};

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const entt::entity entity[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int value = 42;

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);
    data.emplace_back(value);

    loader.get<entt::entity>(archive);
    loader.get<int>(archive);

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_TRUE(registry.valid(entity[1u]));

    loader.orphans();

    ASSERT_TRUE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
}

TEST(BasicContinuousLoader, Constructors) {
    static_assert(!std::is_default_constructible_v<entt::basic_continuous_loader<entt::registry>>);
    static_assert(!std::is_copy_constructible_v<entt::basic_continuous_loader<entt::registry>>);
    static_assert(!std::is_copy_assignable_v<entt::basic_continuous_loader<entt::registry>>);
    static_assert(std::is_move_constructible_v<entt::basic_continuous_loader<entt::registry>>);
    static_assert(std::is_move_assignable_v<entt::basic_continuous_loader<entt::registry>>);

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    entt::basic_continuous_loader other{std::move(loader)};

    ASSERT_NO_FATAL_FAILURE(loader = std::move(other));
}

TEST(BasicContinuousLoader, GetEntityType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<entt::entity>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const entt::entity entity[3u]{traits_type::construct(1u, 0u), traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
    ASSERT_FALSE(registry.valid(entity[2u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(0u));

    loader.get<entt::entity>(archive);

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));
    ASSERT_FALSE(registry.valid(entity[2u]));

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));
    ASSERT_FALSE(loader.contains(entity[2u]));

    ASSERT_EQ(loader.map(entity[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entity[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(storage.in_use(), 0u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);
    data.emplace_back(entity[2u]);

    loader.get<entt::entity>(archive, "ignored"_hs);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));
    ASSERT_FALSE(loader.contains(entity[2u]));

    ASSERT_NE(loader.map(entity[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entity[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_EQ(storage.in_use(), 2u);

    ASSERT_EQ(storage[0u], loader.map(entity[0u]));
    ASSERT_EQ(storage[1u], loader.map(entity[1u]));

    ASSERT_EQ(registry.create(), entity[2u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);
    data.emplace_back(entity[2u]);

    loader.get<entt::entity>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));
    ASSERT_TRUE(loader.contains(entity[2u]));

    ASSERT_NE(loader.map(entity[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entity[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[2u])));

    ASSERT_EQ(storage.size(), 4u);
    ASSERT_EQ(storage.in_use(), 4u);

    ASSERT_EQ(storage[0u], loader.map(entity[0u]));
    ASSERT_EQ(storage[1u], loader.map(entity[1u]));
    ASSERT_EQ(storage[3u], loader.map(entity[2u]));

    registry.destroy(loader.map(entity[1u]));

    ASSERT_TRUE(loader.contains(entity[1u]));
    ASSERT_NE(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));

    data.emplace_back(entity[1u]);

    loader.get<entt::entity>(archive);

    ASSERT_TRUE(loader.contains(entity[1u]));
    ASSERT_NE(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));
    ASSERT_EQ(storage[3u], loader.map(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));

    data.emplace_back(entity[1u]);
    data.emplace_back(entity[2u]);
    data.emplace_back(entity[0u]);

    loader.get<entt::entity>(archive, "ignored"_hs);

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));
    ASSERT_FALSE(loader.contains(entity[2u]));

    ASSERT_EQ(loader.map(entity[0u]), static_cast<entt::entity>(entt::null));
    ASSERT_NE(loader.map(entity[1u]), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(loader.map(entity[2u]), static_cast<entt::entity>(entt::null));

    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 4u);
    ASSERT_EQ(storage.in_use(), 2u);

    ASSERT_EQ(storage[1u], loader.map(entity[1u]));
}

TEST(BasicContinuousLoader, GetType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const entt::entity entity[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int values[2u]{1, 3};

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);
    data.emplace_back(values[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(values[0u]);

    data.emplace_back(entity[1u]);
    data.emplace_back(values[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(loader.map(entity[0u])));
    ASSERT_TRUE(storage.contains(loader.map(entity[1u])));
    ASSERT_EQ(storage.get(loader.map(entity[0u])), values[0u]);
    ASSERT_EQ(storage.get(loader.map(entity[1u])), values[1u]);
}

TEST(BasicContinuousLoader, GetTypeExtended) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<shadow>();

    std::vector<entt::any> data{};
    const entt::entity entity[2u]{traits_type::construct(0u, 1u), traits_type::construct(1u, 1u)};
    const shadow value{entity[0u]};

    auto archive = [&loader, &data, pos = 0u](auto &elem) mutable {
        elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]);

        if constexpr(std::is_same_v<std::remove_reference_t<decltype(elem)>, shadow>) {
            elem.target = loader.map(elem.target);
        }
    };

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[1u]);
    data.emplace_back(value);

    loader.get<entt::entity>(archive);
    loader.get<shadow>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    ASSERT_EQ(storage.size(), 1u);
    ASSERT_TRUE(storage.contains(loader.map(entity[1u])));
    ASSERT_EQ(storage.get(loader.map(entity[1u])).target, loader.map(entity[0u]));
}

TEST(BasicContinuousLoader, GetEmptyType) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<empty>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const entt::entity entity[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);

    loader.get<empty>(archive, "other"_hs);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<empty>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);

    loader.get<empty>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(loader.map(entity[0u])));
    ASSERT_TRUE(storage.contains(loader.map(entity[1u])));
}

TEST(BasicContinuousLoader, GetTypeSparse) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    const auto &storage = registry.storage<int>();

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const entt::entity entity[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int values[2u]{1, 3};

    ASSERT_FALSE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_FALSE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<entt::entity>(entt::null));
    data.emplace_back(entity[0u]);
    data.emplace_back(values[0u]);

    loader.get<int>(archive, "other"_hs);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_FALSE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 0u);
    ASSERT_EQ(registry.storage<int>("other"_hs).size(), 1u);

    data.emplace_back(static_cast<typename traits_type::entity_type>(3u));

    data.emplace_back(entity[0u]);
    data.emplace_back(values[0u]);

    data.emplace_back(static_cast<entt::entity>(entt::null));

    data.emplace_back(entity[1u]);
    data.emplace_back(values[1u]);

    loader.get<int>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    ASSERT_EQ(storage.size(), 2u);
    ASSERT_TRUE(storage.contains(loader.map(entity[0u])));
    ASSERT_TRUE(storage.contains(loader.map(entity[1u])));
    ASSERT_EQ(storage.get(loader.map(entity[0u])), values[0u]);
    ASSERT_EQ(storage.get(loader.map(entity[1u])), values[1u]);
}

TEST(BasicContinuousLoader, GetTypeWithListener) {
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};
    entt::entity check{entt::null};

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &value) mutable { value = entt::any_cast<std::remove_reference_t<decltype(value)>>(data[pos++]); };
    const auto entity{traits_type::construct(1u, 1u)};
    const shadow value{entity};

    ASSERT_FALSE(registry.valid(loader.map(entity)));
    ASSERT_EQ(check, static_cast<entt::entity>(entt::null));

    registry.on_construct<shadow>().connect<&shadow::listener>(check);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity);
    data.emplace_back(value);

    loader.get<shadow>(archive);

    ASSERT_TRUE(registry.valid(loader.map(entity)));
    ASSERT_EQ(check, entity);
}

TEST(BasicContinuousLoader, Orphans) {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_continuous_loader loader{registry};

    std::vector<entt::any> data{};
    auto archive = [&data, pos = 0u](auto &elem) mutable { elem = entt::any_cast<std::remove_reference_t<decltype(elem)>>(data[pos++]); };
    const entt::entity entity[2u]{traits_type::construct(0u, 0u), traits_type::construct(2u, 0u)};
    const int value = 42;

    ASSERT_FALSE(registry.valid(entity[0u]));
    ASSERT_FALSE(registry.valid(entity[1u]));

    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));
    data.emplace_back(static_cast<typename traits_type::entity_type>(2u));

    data.emplace_back(entity[0u]);
    data.emplace_back(entity[1u]);

    data.emplace_back(static_cast<typename traits_type::entity_type>(1u));
    data.emplace_back(entity[0u]);
    data.emplace_back(value);

    loader.get<entt::entity>(archive);
    loader.get<int>(archive);

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_TRUE(registry.valid(loader.map(entity[1u])));

    loader.orphans();

    ASSERT_TRUE(loader.contains(entity[0u]));
    ASSERT_TRUE(loader.contains(entity[1u]));

    ASSERT_TRUE(registry.valid(loader.map(entity[0u])));
    ASSERT_FALSE(registry.valid(loader.map(entity[1u])));
}
