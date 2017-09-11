// JSON simple example
// This example does not handle errors.

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

using namespace rapidjson;

int main() {
    // 1. Parse a JSON string into DOM.
    const char* json = "{\"id\":0,\"result\":{\"value\":[{\"id\":\"USE_1122121\"},{\"id\":\"USE_2\"},{\"id\":\"USE_1\"},{\"id\":\"USE_3\"}], \
\"sessionId\":\"jg30csk6m74jvhmoqc0l279c1h\"},\"jsonrpc\":\"2.0\"}";
    Document d;
    d.Parse(json);
    auto has_value = d.HasParseError();
    if (d.HasMember("result")) {
        Value  &result = d["result1"];

        Value &value = result["value"];

        auto &array = value.GetArray();

        for (auto f = array.Begin(); f != array.End(); ++f) {
            auto ii = (*f)["id"].GetString();
            std::cout << ii << std::endl;
        }
        
    }


    // 3. Stringify the DOM
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);
    // Output {"project":"rapidjson","stars":11}
    std::cout << buffer.GetString() << std::endl;





    return 0;
}
