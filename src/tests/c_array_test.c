#include <cbehave/cbehave.h>

#include <c_array.h>

#include <SDL_joystick.h>

#include <utils.h>

// Stubs
const char *JoyName(const int deviceIndex)
{
	UNUSED(deviceIndex);
	return NULL;
}


FEATURE(CArrayInsert, "Array insert")
	SCENARIO("Insert in middle")
		GIVEN("an array with some elements")
			CArray a;
			CArrayInit(&a, sizeof(int));
			for (int i = 0; i < 5; i++)
			{
				CArrayPushBack(&a, &i);
			}
			size_t oldSize = a.size;

		WHEN("I insert an element in the middle")
			int middleIndex = 3;
			int middleValue = -1;
			CArrayInsert(&a, middleIndex, &middleValue);

		THEN("the array should be one element larger")
			SHOULD_INT_EQUAL((int)a.size, (int)oldSize + 1);
		AND("contain the previous elements, the middle element, plus the next elements")
			for (int i = 0; i < (int)a.size; i++)
			{
				int *v = CArrayGet(&a, i);
				if (i < middleIndex)
				{
					SHOULD_INT_EQUAL(*v, i);
				}
				else if (i == middleIndex)
				{
					SHOULD_INT_EQUAL(*v, middleValue);
				}
				else
				{
					SHOULD_INT_EQUAL(*v, i - 1);
				}
			}
	SCENARIO_END
FEATURE_END

FEATURE(CArrayDelete, "Array delete")
	SCENARIO("Delete from middle")
		GIVEN("an array with some elements")
			CArray a;
			CArrayInit(&a, sizeof(int));
			for (int i = 0; i < 5; i++)
			{
				CArrayPushBack(&a, &i);
			}
			size_t oldSize = a.size;

		WHEN("I delete an element from the middle")
			int middleIndex = 3;
			CArrayDelete(&a, middleIndex);

		THEN("the array should be one element smaller")
			SHOULD_INT_EQUAL((int)a.size, (int)oldSize - 1);
		AND("contain all but the deleted elements in order")
			for (int i = 0; i < (int)a.size; i++)
			{
				int *v = CArrayGet(&a, i);
				if (i < middleIndex)
				{
					SHOULD_INT_EQUAL(*v, i);
				}
				else if (i >= middleIndex)
				{
					SHOULD_INT_EQUAL(*v, i + 1);
				}
			}
	SCENARIO_END
FEATURE_END

static bool LessThan2(const void *elem)
{
	return *(const int *)elem < 2;
}

FEATURE(CArrayRemoveIf, "Array remove if")
	SCENARIO("Remove if")
		GIVEN("an array with numbers 0-4")
			CArray a;
			CArrayInit(&a, sizeof(int));
			for (int i = 0; i < 5; i++)
			{
				CArrayPushBack(&a, &i);
			}

		WHEN("I remove all elements less than 2")
			CArrayRemoveIf(&a, LessThan2);

		THEN("the array should contain the numbers >= 2")
			SHOULD_INT_EQUAL((int)a.size, 3);
			for (int i = 0; i < (int)a.size; i++)
			{
				int *v = CArrayGet(&a, i);
				SHOULD_INT_GE(*v, 2);
			}
	SCENARIO_END
FEATURE_END

static bool IsEqual(const void *e1, const void *e2)
{
	return *(const int *)e1 == *(const int *)e2;
}

FEATURE(CArrayUnique, "Array unique")
	SCENARIO("Remove consecutive numbers")
		GIVEN("an array with numbers 0, 1, 1, 2")
			CArray a;
			CArrayInit(&a, sizeof(int));
			int numbers[] = {0, 1, 1, 2};
			for (int i = 0; i < 4; i++)
			{
				CArrayPushBack(&a, &numbers[i]);
			}

		WHEN("I remove non-unique elements")
			CArrayUnique(&a, IsEqual);

		THEN("the array should contain the numbers 1, 2, 3")
			SHOULD_INT_EQUAL((int)a.size, 3);
			int expected[] = {0, 1, 2};
			CA_FOREACH(const int, v, a)
				SHOULD_INT_EQUAL(*v, expected[_ca_index]);
			CA_FOREACH_END()
	SCENARIO_END
FEATURE_END

FEATURE(CArrayCopy, "Array copy")
	SCENARIO("Copy")
		GIVEN("an array with numbers 0-4")
			CArray a;
			CArrayInit(&a, sizeof(int));
			for (int i = 0; i < 5; i++)
			{
				CArrayPushBack(&a, &i);
			}

		WHEN("I copy the array")
			CArray a2;
			memset(&a2, 0, sizeof a2);
			CArrayCopy(&a2, &a);

		THEN("the array copy should contain 0-4")
			CA_FOREACH(const int, i, a2)
				SHOULD_INT_EQUAL(*i, _ca_index);
			CA_FOREACH_END()
	SCENARIO_END
FEATURE_END

CBEHAVE_RUN(
	"CArray features are:",
	TEST_FEATURE(CArrayInsert),
	TEST_FEATURE(CArrayDelete),
	TEST_FEATURE(CArrayRemoveIf),
	TEST_FEATURE(CArrayUnique),
	TEST_FEATURE(CArrayCopy)
)
