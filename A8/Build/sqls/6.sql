
select
        l.l_shipmode,
        sum (1)
from
        orders as o,
        lineitem as l
where
        (o.o_orderkey = l.l_orderkey)
        and (o.o_orderstatus = "F")
        and (l.l_shipmode = "TRUCK" or l.l_shipmode = "RAIL")
        and (l.l_commitdate < l.l_receiptdate)
        and (l.l_shipdate < l.l_commitdate)
group by
        l.l_shipmode;
