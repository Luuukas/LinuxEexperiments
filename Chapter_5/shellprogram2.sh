if [ $# = 3 ]
then
    case $2 in
    +) let z=$1+$3;;
    -) let z=$1-$3;;
    /) let z=$1/$3;;
    x|X) let z=$1*$3;;
    *) echo "Waring-$2 invalied operator, only +, -, *, / operator allowed."
       exit;;
    esac
        echo "Answer is $z"
else
    echo "Usage-$0 value operator value2."
fi