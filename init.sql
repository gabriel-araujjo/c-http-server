CREATE DATABASE db;
GRANT ALL PRIVILEGES ON DATABASE db TO admin;
\connect db;

CREATE TABLE IF NOT EXISTS clientes (
    id smallint PRIMARY KEY,
    balance integer NOT NULL
);
GRANT ALL PRIVILEGES ON TABLE clientes TO ADMIN;

DO $$
BEGIN
    INSERT INTO clientes(id, balance)
    VALUES
        (1, 56789);
END; $$
