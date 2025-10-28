#include "core.h"

#include <algorithm>
#include <charconv>

namespace platformer2d::Core {

	static void SkipWhitespaceAndCommas(const char*& Ptr, const char* End)
	{
		while ((Ptr < End) 
			   && ((*Ptr == ' ') 
			   || (*Ptr == '\t') 
			   || (*Ptr == '\n') 
			   || (*Ptr == '\r') 
			   || (*Ptr == ',')))
		{
			++Ptr;
		}
	}

	static bool ReadFloat(const char*& Ptr, const char* End, float& OutValue)
	{
		SkipWhitespaceAndCommas(Ptr, End);
		if (Ptr >= End)
		{
			return false;
		}

		std::from_chars_result Result = std::from_chars(Ptr, End, OutValue);
		if (Result.ec != std::errc())
		{
			return false;
		}

		Ptr = Result.ptr;
		SkipWhitespaceAndCommas(Ptr, End);

		return true;
	}

	int ParseSvgPath(std::string_view SvgPath, const glm::vec2& Offset,
					 std::span<glm::vec2> Points, const float Scale, const bool ReverseOrder)
	{
		const char* Ptr = SvgPath.data();
		const char* End = SvgPath.data() + SvgPath.size();

		int PointCount = 0;
		glm::vec2 CurrentPoint{ 0.0f, 0.0f };
		char Command = '\0';

		SkipWhitespaceAndCommas(Ptr, End);

		while (Ptr < End)
		{
			if (((*Ptr >= 'A') && (*Ptr <= 'Z'))
				|| ((*Ptr >= 'a') && (*Ptr <= 'z')))
			{
				Command = *Ptr++;
				SkipWhitespaceAndCommas(Ptr, End);

				if ((Command == 'z') || (Command == 'Z'))
				{
					break;
				}
			}

			/* All supported commands require at least one number (except z/Z handled above). */
			float X = 0.0f;
			float Y = 0.0f;

			switch (Command)
			{
				case 'M':
				case 'L':
				{
					const bool OkX = ReadFloat(Ptr, End, X);
					const bool OkY = ReadFloat(Ptr, End, Y);
					LK_ASSERT(OkX && OkY);
					CurrentPoint.x = X;
					CurrentPoint.y = Y;
				}
				break;

				case 'H':
				{
					const bool OkX = ReadFloat(Ptr, End, X);
					LK_ASSERT(OkX);
					CurrentPoint.x = X;
				}
				break;

				case 'V':
				{
					const bool OkY = ReadFloat(Ptr, End, Y);
					LK_ASSERT(OkY);
					CurrentPoint.y = Y;
				}
				break;

				case 'm':
				case 'l':
				{
					const bool OkX = ReadFloat(Ptr, End, X);
					const bool OkY = ReadFloat(Ptr, End, Y);
					LK_ASSERT(OkX && OkY);
					CurrentPoint.x += X;
					CurrentPoint.y += Y;
				}
				break;

				case 'h':
				{
					const bool OkX = ReadFloat(Ptr, End, X);
					LK_ASSERT(OkX);
					CurrentPoint.x += X;
				}
				break;

				case 'v':
				{
					const bool OkY = ReadFloat(Ptr, End, Y);
					LK_ASSERT(OkY);
					CurrentPoint.y += Y;
				}
				break;

				default:
					LK_VERIFY(false);
					return PointCount;
			}

			if (PointCount < static_cast<int>(Points.size()))
			{
				/**
				 * SVG Y increases downward. 
				 * Negate Y to map to typical math coordinates if desired.
				 */
				Points[static_cast<std::size_t>(PointCount)] = {
					Scale * (CurrentPoint.x + Offset.x),
					-Scale * (CurrentPoint.y + Offset.y)
				};
				PointCount += 1;
			}
			else
			{
				break;
			}

			SkipWhitespaceAndCommas(Ptr, End);
		}

		if (PointCount == 0)
		{
			return 0;
		}

		if (ReverseOrder)
		{
			std::reverse(Points.begin(), Points.begin() + PointCount);
		}

		return PointCount;
	}

}
