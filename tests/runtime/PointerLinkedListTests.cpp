#include "PointerLinkedList.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <iterator>
#include <ranges>
#include <vector>

namespace
{
	struct Node
	{
		std::int64_t value;
		Node* next;
	};

	std::vector<std::int64_t> Values(const PointerLinkedList<Node>& list)
	{
		std::vector<std::int64_t> values;
		for (const auto& node : list) {
			values.push_back(node.value);
		}
		return values;
	}
}

static_assert(std::forward_iterator<PointerLinkedList<Node>::Iterator>);
static_assert(std::ranges::forward_range<PointerLinkedList<Node>>);

TEST_CASE("PointerLinkedList iterates the nodes in order", "[PointerLinkedList]")
{
	Node third{ 3, nullptr };
	Node second{ 2, &third };
	Node first{ 1, &second };
	Node* head = &first;
	PointerLinkedList list(&head);

	CHECK(Values(list) == std::vector<std::int64_t>{ 1, 2, 3 });
}

TEST_CASE("PointerLinkedList over a null head is empty", "[PointerLinkedList]")
{
	Node* head = nullptr;
	PointerLinkedList<Node> list(&head);

	CHECK(list.begin() == list.end());
	CHECK(list.remove(list.begin()) == list.end());
}

TEST_CASE("PointerLinkedList::remove unlinks a node and returns the next one", "[PointerLinkedList]")
{
	Node third{ 3, nullptr };
	Node second{ 2, &third };
	Node first{ 1, &second };
	Node* head = &first;
	PointerLinkedList list(&head);

	SECTION("head")
	{
		auto next = list.remove(list.begin());
		CHECK(head == &second);
		CHECK(next->value == 2);
		CHECK(Values(list) == std::vector<std::int64_t>{ 2, 3 });
	}

	SECTION("middle")
	{
		auto next = list.remove(std::next(list.begin()));
		CHECK(next->value == 3);
		CHECK(Values(list) == std::vector<std::int64_t>{ 1, 3 });
	}

	SECTION("tail")
	{
		auto next = list.remove(std::next(list.begin(), 2));
		CHECK(next == list.end());
		CHECK(Values(list) == std::vector<std::int64_t>{ 1, 2 });
	}

	SECTION("node that is not in the list")
	{
		Node stranger{ 4, nullptr };
		CHECK(list.remove(PointerLinkedList<Node>::Iterator(&stranger)) == list.end());
		CHECK(Values(list) == std::vector<std::int64_t>{ 1, 2, 3 });
	}
}

TEST_CASE("PointerLinkedList erase loop removes matching nodes", "[PointerLinkedList]")
{
	Node fourth{ 4, nullptr };
	Node third{ 3, &fourth };
	Node second{ 2, &third };
	Node first{ 1, &second };
	Node* head = &first;
	PointerLinkedList list(&head);

	for (auto it = list.begin(); it != list.end();) {
		it = it->value % 2 == 0 ? list.remove(it) : std::next(it);
	}

	CHECK(Values(list) == std::vector<std::int64_t>{ 1, 3 });
}
