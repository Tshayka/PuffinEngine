#pragma once

#include <gtest/gtest.h>

#include "../puffinEngine/src/PuffinEngine.cpp"
#include "../puffinEngine/src/Device.cpp"

class PuffinEngineTest : public ::testing::Test
{
public:
    PuffinEngine uut;
};