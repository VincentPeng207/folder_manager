#ifndef UUID_GENERATOR_H
#define UUID_GENERATOR_H

#include <string>

// Simple utility class to generate UUID strings
class UuidGenerator {
public:
    // Generate a new UUID string
    static std::string generate();
    
    // Alias for generate() to match naming convention in other files
    static std::string generateUuid() { return generate(); }
};

#endif // UUID_GENERATOR_H 