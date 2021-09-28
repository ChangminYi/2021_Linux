# 작성자: 20170454 이창민

# echo $1 $2
# 0번 인자는 자기 자신의 상대적 위치

# 인자가 2개가 아니면 에러
if [ $# -ne 2 ]; then
	echo 'Error: 2 parameters are needed.'
# 1번 인자 범위 체크
elif [ $1 -lt 1 ]; then
	echo 'Error: first parameter is lower than 1.'
# 2번 인자 범위 체크
elif [ $2 -lt 1 ]; then
	echo 'Error: second parameter is lower than 1.'
# 정상 실행
else
	i=1
	
	while [ $i -le $1 ]
	do
		j=1
	
		while [ $j -le $2 ]
		do
			echo -n $i' * '$j' = '`expr $i \* $j`
			if [ $j == $2 ]; then
				echo ''
			else
				echo -n ' '
			fi
			
			j=`expr $j + 1`
		done
		
		i=`expr $i + 1`
	done
fi
