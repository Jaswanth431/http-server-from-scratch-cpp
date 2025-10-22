#pragma once
#include<request.h>
#include<response.h>
#include<functional>

using middelwareFunction = std::function<void(Request&, Response&)>;
