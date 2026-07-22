-- schema.sql — database + contacts table for mon1mm
--
-- Run with:  mysql -u root -p < schema.sql

CREATE DATABASE IF NOT EXISTS n1mm
	CHARACTER SET utf8mb4
	COLLATE utf8mb4_unicode_ci;

USE n1mm;

CREATE TABLE IF NOT EXISTS contacts (
	ID              VARCHAR(50)   NOT NULL,
	app             VARCHAR(100),
	contestname     VARCHAR(100),
	contestnr       INT,
	timestamp       DATETIME,
	mycall          VARCHAR(50),
	band            VARCHAR(20),
	rxfreq          DECIMAL(15,2),
	txfreq          DECIMAL(15,2),
	operator        VARCHAR(50),
	mode            VARCHAR(20),
	call            VARCHAR(50),
	countryprefix   VARCHAR(20),
	wpxprefix       VARCHAR(20),
	stationprefix   VARCHAR(50),
	continent       VARCHAR(10),
	snt             VARCHAR(20),
	sntnr           INT,
	rcv             VARCHAR(20),
	rcvnr           INT,
	gridsquare      VARCHAR(20),
	exchange1       VARCHAR(50),
	section         VARCHAR(20),
	comment         VARCHAR(255),
	qth             VARCHAR(100),
	name            VARCHAR(100),
	power           VARCHAR(20),
	misc            VARCHAR(255),
	zone            INT,
	prec            VARCHAR(10),
	ck              INT,
	ismultiplier1   INT,
	ismultiplier2   INT,
	ismultiplier3   INT,
	points          INT,
	radionr         INT,
	run1run2        INT,
	RoverLocation   VARCHAR(50),
	RadioInterfaced INT,
	NetworkedCompNr INT,
	IsOriginal      VARCHAR(20),
	NetBiosName     VARCHAR(100),
	IsRunQSO        INT,
	Frequency       DECIMAL(15,2),
	IsClaimedQso    INT,
	updated_at      TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
						ON UPDATE CURRENT_TIMESTAMP,
	PRIMARY KEY (ID),
	KEY idx_call (call),
	KEY idx_contestnr (contestnr),
	KEY idx_timestamp (timestamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
