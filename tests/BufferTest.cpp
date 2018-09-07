#include "BufferTest.hpp"

TEST_F(BufferTest, BasicParametersOfBuffer){
    EXPECT_EQ(0, uut.size);
    EXPECT_EQ(0, uut.alignment);
}