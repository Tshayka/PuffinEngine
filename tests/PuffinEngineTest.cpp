#include "PuffinEngineTest.hpp"

TEST_F(PuffinEngineTest, Dummy){
    ASSERT_EQ("This letter is lowercase!", uut.isUppercase('a'));
}