var x, y, z, xx, yy, zz;
begin
    x := 1;
    y := 2;
    z := 3;
    xx := x;
    yy := y;
    zz := z;
    x := y + z + xx + yy + zz + x;
    xx := xx + yy + zz + 1 + x + y + z + x
end.