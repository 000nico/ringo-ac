#include "core/api/server.hpp"
#include "core/db/db.hpp"

int main() {
    db_init("ringo.db");

    RingoServer server("0.0.0.0", 8080);
    server.start();
    return 0;
}