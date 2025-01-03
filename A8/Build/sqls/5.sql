
select
        "return flag was " + l.l_returnflag,
        sum (l.l_quantity),
        sum (l.l_extendedprice),
        sum (l.l_extendedprice*(1-l.l_discount)),
        sum (l.l_extendedprice*(1-l.l_discount)*(1+l.l_tax)),
        avg (l.l_quantity),
        avg (l.l_extendedprice),
        avg (l.l_discount),
        sum (1)
from
        lineitem as l
where
        (l.l_shipdate < "1998-12-01") and
        (l.l_shipdate > "1998-06-01")
group by
        l.l_returnflag;
