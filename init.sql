CREATE DATABASE db;
GRANT ALL PRIVILEGES ON DATABASE db TO admin;
\connect db;

CREATE TABLE IF NOT EXISTS clientes (
    id smallint PRIMARY KEY,
    limite integer NOT NULL,
    saldo integer NOT NULL
);
GRANT ALL PRIVILEGES ON TABLE clientes TO ADMIN;

CREATE TABLE IF NOT EXISTS extrato (
    id integer PRIMARY KEY GENERATED ALWAYS as IDENTITY,
    id_cliente smallint NOT NULL,
    valor integer NOT NULL,
    tipo char(1) NOT NULL,
    descricao varchar(10) NOT NULL,
    realizada_em timestamp NOT NULL,
    CONSTRAINT fk_clientes FOREIGN KEY(id_cliente) REFERENCES clientes(id)
);
GRANT ALL PRIVILEGES ON TABLE extrato TO ADMIN;

DO $$
BEGIN
    INSERT INTO clientes(id, limite, saldo)
    VALUES
        (1, 100000, 0),
        (2, 80000, 0),
        (3, 1000000, 0),
        (4, 10000000, 0),
        (5, 500000, 0);
END; $$
