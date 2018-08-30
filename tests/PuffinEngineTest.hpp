#pragma once

#include <gtest/gtest.h>

#include "../puffinEngine/src/PuffinEngine.cpp"

class PuffinEngineTest : public ::testing::Test
{
public:
    PuffinEngine uut;
};