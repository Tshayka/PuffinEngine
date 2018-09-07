#pragma once

#include <gtest/gtest.h>

#include "../puffinEngine/src/Buffer.cpp"

class BufferTest : public ::testing::Test
{
public:
    enginetool::Buffer uut;
};