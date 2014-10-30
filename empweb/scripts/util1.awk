{
split($5, d, ".")
c = sprintf("%d.%d.%d.%d", d[1],d[2],d[3],d[4])
split(d[5], d1, ":")
print c, $1, d1[1], $3, $6, $7, $8, $9, $10, $11
}
