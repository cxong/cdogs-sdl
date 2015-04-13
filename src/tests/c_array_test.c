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


FEATURE(1, "Array insert")
	SCENARIO("Insert in middle")
	{
		CArray a;
		GIVEN("an array with some elements")
			CArrayInit(&a, sizeof(int));
			for (int i = 0; i < 5; i++)
			{
				CArrayPushBack(&a, &i);
			}
		GIVEN_END
		size_t oldSize = a.size;

		int middleIndex = 3;
		int middleValue = -1;
		WHEN("I insert an element in the middle")
			CArrayInsert(&a, middleIndex, &middleValue);
		WHEN_END

		THEN("the array should be one element larger, and contain the previous elements, the middle element, plus the next elements");
			SHOULD_INT_EQUAL((int)a.size, (int)oldSize + 1);
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
		THEN_END
	}
	SCENARIO_END
FEATURE_END

FEATURE(2, "Array delete")
	SCENARIO("Delete from middle")
	{
		CArray a;
		GIVEN("an array with some elements")
			CArrayInit(&a, sizeof(int));
			for (int i = 0; i < 5; i++)
			{
				CArrayPushBack(&a, &i);
			}
		GIVEN_END
		size_t oldSize = a.size;

		int middleIndex = 3;
		WHEN("I delete an element from the middle")
			CArrayDelete(&a, middleIndex);
		WHEN_END

		THEN("the array should be one element smaller, and contain all but the deleted elements in order");
			SHOULD_INT_EQUAL((int)a.size, (int)oldSize - 1);
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
		THEN_END
	}
	SCENARIO_END
FEATURE_END

static bool LessThan2(const void *elem)
{
	return *(const int *)elem < 2;
}

FEATURE(3, "Array remove if")
	SCENARIO("Remove if")
	{
		CArray a;
		GIVEN("an array with numbers 0-4")
			CArrayInit(&a, sizeof(int));
			for (int i = 0; i < 5; i++)
			{
				CArrayPushBack(&a, &i);
			}
		GIVEN_END

		WHEN("I remove all elements less than 2")
			CArrayRemoveIf(&a, LessThan2);
		WHEN_END

		THEN("the array should contain the numbers >= 2");
			SHOULD_INT_EQUAL((int)a.size, 3);
			for (int i = 0; i < (int)a.size; i++)
			{
				int *v = CArrayGet(&a, i);
				SHOULD_INT_GE(*v, 2);
			}
		THEN_END
	}
	SCENARIO_END
FEATURE_END

int main(void)
{
	cbehave_feature features[] =
	{
		{feature_idx(1)},
		{feature_idx(2)},
		{feature_idx(3)}
	};
	
	return cbehave_runner("CArray features are:", features);
}
