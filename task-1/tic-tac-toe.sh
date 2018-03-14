#!/bin/bash

BORDER_COLOR=`tput setaf 7`
CURSOR_COLOR=`tput setaf 4`
NORMAL_COLOR=`tput sgr0`

MODEL='         '
CURSOR_X=0
CURSOR_Y=0
CHAR='o'
TURN='x'

function draw_model() {
    b=$BORDER_COLOR
    n=$NORMAL_COLOR
    c=$CURSOR_COLOR

    tput reset
    echo ${b}'Your character: '${n}${CHAR}
    echo ${b}'Turn of: ' ${n}${TURN}$'\n'
    echo ${b}'┏━━━┳━━━┳━━━┓'

    for i in 0 1 2; do
        for j in 0 1 2; do
            m=${MODEL:3 * i + j:1}

            if [[ $i = $CURSOR_Y ]] && [[ $j = $CURSOR_X ]];
                then echo -n '┃'${c}'('${n}${m}${c}')'${b}
                else echo -n '┃ '${n}${m}' '${b}
            fi
        done

        echo '┃ '

        if [[ $i != 2 ]];
            then echo '┣━━━╋━━━╋━━━┫'
            else echo '┗━━━┻━━━┻━━━┛'
        fi
    done

    echo $n
}

function move_cursor_up() {
    CURSOR_Y=$(((CURSOR_Y + 2) % 3))
}

function move_cursor_down() {
    CURSOR_Y=$(((CURSOR_Y + 1) % 3))
}

function move_cursor_left() {
    CURSOR_X=$(((CURSOR_X + 2) % 3))
}

function move_cursor_right() {
    CURSOR_X=$(((CURSOR_X + 1) % 3))
}

function fill_cell() {
    x=$2
    y=$3
    cursor=$((3 * y + x))

    if [[ ${MODEL:cursor:1} = ' ' ]]; then
        MODEL=${MODEL:0:cursor}${1}${MODEL:cursor + 1}
    fi
}

function put_my_char() {
    fill_cell $CHAR $CURSOR_X $CURSOR_Y
    echo $CURSOR_X $CURSOR_Y > channel
    TURN=`enemy_char`
}

function make_move() {
    read -r -sn1 t

    case $t in
        A) move_cursor_up;;
        B) move_cursor_down;;
        C) move_cursor_right;;
        D) move_cursor_left;;
        '') put_my_char;;
    esac
}

function wait_enemy_move() {
    enemy_move=`cat channel`
    fill_cell `enemy_char` $enemy_move
    TURN=$CHAR
}

function is_finished() {
    ways_to_win=(0 1 2 3 4 5 6 7 8 0 3 6 1 4 7 2 5 8 0 4 8 2 4 6)

    for i in `seq 0 3 $((${#ways_to_win[@]} - 1))`; do
        a=${MODEL:ways_to_win[i]:1}
        b=${MODEL:ways_to_win[i + 1]:1}
        c=${MODEL:ways_to_win[i + 2]:1}

        if [[ $a = $b ]] && [[ $b = $c ]] && [[ $a != '-' ]]; then
            echo $a
            break
        fi
    done
}

function random_char() {
    r=$((RANDOM % 2))

    if [[ $r = 0 ]];
        then echo 'x';
        else echo 'o';
    fi
}

function enemy_char() {
    if [[ $CHAR = 'x' ]];
        then echo 'o'
        else echo 'x'
    fi
}

function connect_first_player() {
    trap 'rm channel; reset' EXIT
    echo 'Waiting for the second player...'
    
    CHAR=`random_char`
    echo `enemy_char` > channel

    enemy_pid=`cat channel`
    trap 'kill -INT -'$enemy_pid' &>/dev/null; reset; exit' INT

    echo $$ > channel
}

function connect_second_player() {
    CHAR=`cat channel`
    
    echo $$ > channel

    enemy_pid=`cat channel`
    trap 'kill -INT -'$enemy_pid' &>/dev/null; reset; exit' INT
    trap 'reset' EXIT
}

function exit_if_finished() {
    winner=`is_finished`

    if [[ $winner != '' ]] || [[ ! $MODEL =~ " " ]]; then
        if [[ $winner = $CHAR ]];
            then echo 'You win!'
        elif [[ $winner = `enemy_char` ]];
            then echo 'You lose!'
        fi

        sleep 5
        exit
    fi
}

function main() {
    stty -echo    
    mkfifo channel &>/dev/null

    if [[ $? = 0 ]]; 
        then connect_first_player
        else connect_second_player
    fi

    while true; do
        draw_model

        if [[ $TURN = $CHAR ]]; 
            then make_move
            else wait_enemy_move
        fi

        exit_if_finished
    done
}

main
