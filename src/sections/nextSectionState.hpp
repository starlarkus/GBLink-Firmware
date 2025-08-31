#pragma once

enum class NextSection
{
    setup,
    connection,
    disconnect,
    lounge,
    exit,
    cancel // same as exit, but different reason
};