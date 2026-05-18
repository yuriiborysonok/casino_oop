#include "IoCContainer.hpp"
#include <gtest/gtest.h>

TEST(IoCTests, ResolvesDependencies) {
    // Set a dummy database URL to prevent active socket initialization blocks
    setenv("DATABASE_URL", "dbname=mock_db user=postgres password=secret host=invalid_host port=5432", 1);
    
    IoCContainer container;
    
    auto logger = container.getLogger();
    EXPECT_NE(logger, nullptr);
    
    auto db = container.getDatabase();
    EXPECT_NE(db, nullptr);
}
