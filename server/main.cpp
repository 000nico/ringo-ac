#include "core/api/server.hpp"
#include "core/db/db.hpp"

int main() {
    db_init("ringo.db");

    RINGO_SERVER server("0.0.0.0", 8080);
    server.start();
    return 0;
}