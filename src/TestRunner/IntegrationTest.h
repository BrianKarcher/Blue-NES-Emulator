#pragma once
#include <string>

class IntegrationRunner;

class IntegrationTest
{
public:
    virtual ~IntegrationTest() = default;
    virtual void Setup(IntegrationRunner& runner) = 0;
    virtual void Update() = 0;
    virtual bool IsComplete() = 0;
    virtual bool WasSuccessful() = 0;
    virtual std::string GetName() const = 0;
};