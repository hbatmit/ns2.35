# Define a C string variable of the name $name
#  and set its value to what can be read from stdin

set name [lindex $argv 0]
gets stdin version
puts "char $name\[\] = \"$version\";"
