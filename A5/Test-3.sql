
(1) 
SELECT
        n.n_name,
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l,
        supplier AS s,
        nation AS n,
        region AS r
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.o_orderkey)
        AND (l.l_suppkey = s.s_suppkey)
        AND (c.c_nationkey = s.s_nationkey)
        AND (s.s_nationkey = n.n_nationkey)
        AND (n.n_regionkey = r.r_regionkey)
        AND (r.r_name = "region")
        AND (o.o_orderdate > "date1" OR o.o_orderdate = "date1")
        AND (NOT o.o_orderdate < "date2")
GROUP BY
        n.n_name;
-- passed
(2)
SELECT 
	n.n_name,
	s.s_suppkey,
	SUM(l.l_extendedprice)
FROM 
	nation AS n,
	supplier AS s,
	lineitem AS l
WHERE 
	(l.l_suppkey = s.s_suppkey)
	AND (s.s_nationkey = n.n_nationkey)
GROUP BY
	n.n_name;
-- Syntax error: Select: When grouping, you can only select aggregates or functions of the grouping attributes
(3)
SELECT
	SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
	lineitem AS l
WHERE
	(l.l_extendedprice * (1 - l.l_discount) > "a string");
-- Syntax error: GTOps: can't compare int and string.
(4)
SELECT
	SUM(l.l_returnflag + l.l_shipdate + l.l_commitdate)
FROM 
	lineitem AS l;
-- Syntax error: SumOps: can not apply sum operation on [string].
(5)
SELECT
	l.l_returnflag + l.l_shipdate + l.l_commitdate
FROM 
	lineitem AS l
WHERE
	(l.l_shipdate > "a date"); 
-- passed
(6)
SELECT
        n.n_name,
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l,
        supplier AS s,
        nation AS n,
        region AS r
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.o_orderkey)
        AND (l.l_suppkey = s.s_suppkey)
        AND (c.c_nationkey = s.s_nationkey)
        AND (s.s_nationkey = n.n_nationkey)
        AND (n.n_regionkey = r.r_regionkey)
        AND (r.r_name = "region")
        AND (l.l_tax + l.l_discount > "date2" OR o.o_orderdate = "date1")
        AND (NOT o.o_orderdate < "date2")
GROUP BY
        n.n_name;
-- Syntax error: GTOps: can't compare [int] and [string].
(7)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.k_orderkey);
-- Could not find attribute k_orderkey
-- Candidates were: 
--         o_orderkey
--         o_custkey
--         o_orderstatus
--         o_totalprice
--         o_orderdate
--         o_orderpriority
--         o_clerk
--         o_shippriority
--         o_comment
-- Syntax error: no attribute [k_orderkey] in Table [orders].
(8)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = h.o_custkey)
        AND (l.l_orderkey = o.o_orderkey);
-- Syntax error: no Table referenced as h.
(9)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
	AND ("this is a string" + "this is another string" > 15.0 - "here is another")
        AND (l.l_orderkey = o.o_orderkey);
-- Syntax error: MinusOps: can not minus [double] with [string].
(10)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customers AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.o_orderkey);
-- Syntax error: Error: Table [customers] does not exist.
(11)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.o_orderkey)
	AND (1200 + l.l_quantity / (300.0 + 34) = 33);
-- passed
(12)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
	AND ("this is a string" + "this is another string" > "here is another")
        AND (l.l_orderkey = o.o_orderkey);
-- passed
(13)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount)),
	SUM(c.l_extendedprice * 1.01)
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.o_orderkey);
-- Could not find attribute l_extendedprice
-- Candidates were: 
--         c_custkey
--         c_name
--         c_address
--         c_nationkey
--         c_phone
--         c_acctbal
--         c_mktsegment
--         c_comment
-- Syntax error: no attribute [l_extendedprice] in Table [customer].
(14)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
	AND ("1204" + "this is a string" = l.l_returnflag)
	AND (l.l_tax + l.l_discount + l.l_extendedprice > 3.27)
        AND (l.l_orderkey = o.o_orderkey);
-- passed
(15)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
	AND (l.l_tax + l.l_discount + l.l_extendedprice > 3.27 OR 
		l.l_discount + l.l_extendedprice > "327")
        AND (l.l_orderkey = o.o_orderkey);
-- Syntax error: GTOps: can't compare [int] and [string].
(16)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount)),
	SUM(1 - l.l_discount)
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
	AND (l.l_tax + l.l_discount + l.l_extendedprice > 3.27 OR 
		"here is a string" > "327")
        AND (l.l_orderkey = o.o_orderkey);
-- passed
(17)
SELECT
	SUM(12 + 13.4 + 19),
	SUM(l.l_extendedprice)
FROM 
	lineitem AS l;
-- passed