-- 为已有数据库补充 problems.difficulty 字段（可重复执行）
-- 用法：Get-Content scripts\migrate_add_difficulty.sql -Raw | .\.tools\mysql\bin\mysql.exe -u root

USE oj;

SET @column_exists := (
    SELECT COUNT(*)
    FROM information_schema.COLUMNS
    WHERE TABLE_SCHEMA = 'oj'
      AND TABLE_NAME = 'problems'
      AND COLUMN_NAME = 'difficulty'
);

SET @statement := IF(
    @column_exists = 0,
    'ALTER TABLE problems ADD COLUMN difficulty ENUM(''easy'', ''medium'', ''hard'') NOT NULL DEFAULT ''easy'' AFTER description',
    'SELECT ''difficulty column already exists'' AS migration_note'
);

PREPARE migration_stmt FROM @statement;
EXECUTE migration_stmt;
DEALLOCATE PREPARE migration_stmt;
