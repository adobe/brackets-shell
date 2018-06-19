#!/bin/sh

EMPTY_STRING=""

APP_NAME=$EMPTY_STRING
APP_DIR=$EMPTY_STRING
LOG_FILE=$EMPTY_STRING
MOUNT_POINT=$EMPTY_STRING
TEMP_DIR=$EMPTY_STRING
PID=$EMPTY_STRING

#Return value codes
SUCCESS=0
OPTIONS_MISSING=1

# Logs info messages into log file
function printInfo(){
    currentTime=$(date)
    echo "$currentTime $1" >> $LOG_FILE
    return 0
}

# Logs warning messages into log file
function printWarning(){
    currentTime=$(date)
    echo "$currentTime Warning: $1" >> $LOG_FILE
    return 0
}

# Logs error messages into log file
function printError(){
    currentTime=$(date)
    echo "$currentTime ERROR: $1" >> $LOG_FILE
    return 0
}

# Verifies the setup for the update process
function verifySetup(){
    if [ "$APP_DIR" == "$EMPTY_STRING" ]
    then
        printError "Brackets app dir is missing: INS_O1"
        return $OPTIONS_MISSING
    fi

    if [ "$APP_NAME" == "$EMPTY_STRING" ]
    then
        printError "Brackets app is missing: INS_02"
        return $OPTIONS_MISSING
    fi
    
    if [ "$LOG_FILE" == "$EMPTY_STRING" ]
    then
        printError "Installer log file is missing: INS_03"
        return $OPTIONS_MISSING
    fi
    
    if [ "$MOUNT_POINT" == "$EMPTY_STRING" ]
    then
        printError "DMG mount point is missing: INS_04"
        return $OPTIONS_MISSING
    fi
    
    if [ "$TEMP_DIR" == "$EMPTY_STRING" ]
    then
        printError "Temp directory is missing: INS_05"
        return $OPTIONS_MISSING
    fi

     if [ "$PID" == "$EMPTY_STRING" ]
    then
        printError "PID of current Brackets is missing: INS_06"
        return $OPTIONS_MISSING
    fi
  
    return $SUCCESS
}

# Updates the brackets app
function updateBrackets(){

    printInfo "Check and wait if Brackets is still running ..."

    lsof -p $PID +r 1 &>/dev/null
    exitStatus=$?
    
    # If Brackets is not running anymore : lsof returns 1 if process is not running, and 0 if it exited successfully while being waited
   if [ $exitStatus -eq 1 ] || [ $exitStatus -eq 0 ]
    then
        printInfo "Brackets not running anymore."
        
        local abortInstallation=0
        

        if [ -d $APP_DIR ]
        then
            rm -rf "$TEMP_DIR/$APP_NAME"
            printInfo "Moving the existing Brackets.app to temp dir updateTemp..."
            mv "$APP_DIR/$APP_NAME" "$TEMP_DIR/$APP_NAME"
            
            exitStatus=$?
            if [ $exitStatus -ne 0 ]
            then
                printError "Unable to move the existing Brackets app to temp directory updateTemp. mv retured $exitStatus : INS_07"
                abortInstallation=1
            else
                printInfo "Move Successful"
                
            fi
        fi

        if [ $abortInstallation -eq 0 ]
        then
            printInfo "Copying new Brackets.app ..."
            
            cp -r "$MOUNT_POINT/Brackets.app" "$APP_DIR/$APP_NAME"
            exitStatus=$?
            if [ $exitStatus -ne 0 ]
            then
                printError "Unable to copy Brackets.app. cp retured $exitStatus : INS_08"
                abortInstallation=1
            else
                printInfo "Brackets.app is copied."
                
                printInfo "Verifying code signature of the new app."
                codesign -vvvv "$APP_DIR/$APP_NAME" > /dev/null 2>&1
                exitStatus=$?

                if [ $exitStatus -ne 0 ]
                then
                    printError "Unable to verify the downloaded $APP_DIR/$APP_NAME. codesign retured $exitStatus : INS_09"
                    abortInstallation=1

                    printInfo "Deleting the new app copied."
                    rm -f $APP_DIR/$APP_NAME
                    exitStatus=$?
                    if [ $exitStatus -ne 0 ]
                    then
                        printError "Unable to delete $APP_DIR/$APP_NAME. rm retured $exitStatus : INS_10"
                    else
                        printInfo "Moving the old brackets back from AppData"
                        mv "$TEMP_DIR/$APP_NAME" "$APP_DIR/$APP_NAME"
                        exitStatus=$?
                        if [ $exitStatus -ne 0 ]
                        then
                            printError "Unable to move the old brackets back from AppData. mv retured $exitStatus : INS_11"
                        fi
                    fi
                else

                    printInfo "Code signature verified successfully."
                    printInfo "Marking Brackets as a trusted application..."
                    xattr -d -r com.apple.quarantine "$APP_DIR/$APP_NAME"
                    exitStatus=$?
                    if [ $exitStatus -ne 0 ]
                    then
                        printWarning "Unable to mark Brackets.app as trusted application. You may have to do it manually. xattr returnd $exitStatus."
                    else
                        printInfo "Brackets.app is marked as trusted application."
                    
                    fi
                fi
            fi
        fi

        printInfo "Unmounting the DMG ..."
        hdiutil detach "$MOUNT_POINT" "-force" 
        exitStatus=$?
        if [ $exitStatus -ne 0 ]
        then
            printWarning "DMG could not be unmounted. hdiutil returnd $exitStatus."
        else
            printInfo "Unmounted the DMG and attempying to open Brackets ..."
        fi
        
        rm -rf "$TEMP_DIR/$APP_NAME"
        open -a "$APP_DIR/$APP_NAME"
        exitStatus=$?
        if [ $exitStatus -ne 0 ]
        then
            printError "Brackets could not be opened. open returnd $exitStatus : INS_12"
        else
            printInfo "Opened Brackets."
        fi
        
    else
        printError "Brackets could not be exited properly. lsof returned $exitStatus : INS_13"
    fi

}

# Parse the command line arguments
# -a - App name for currently installed Brackets
# -b - Absolute path to the currently installed Brackets App Directory
# -l - Absolute path to the updation log file path
# -m - Mount Point for the mounted disk image for latest Brackets version
# -t - Absolute path to the update temp directory in AppData
# -p - PID of the currently running brackets process

while getopts a:b:l:m:t:p: OPT; do
    case "$OPT" in
        a)            
            if [ "$APP_NAME" == "$EMPTY_STRING" ]
            then
                APP_NAME=$OPTARG
            fi
            ;;
        b)
            if [ "$APP_DIR" == "$EMPTY_STRING" ]
            then
                APP_DIR=$OPTARG
            fi
            ;;
        l)
            if [ "$LOG_FILE" == "$EMPTY_STRING" ]
            then
                LOG_FILE=$OPTARG
            fi
            ;;
        m)
            if [ "$MOUNT_POINT" == "$EMPTY_STRING" ]
            then
                MOUNT_POINT=$OPTARG
            fi
            ;;
        t)
            if [ "$TEMP_DIR" == "$EMPTY_STRING" ]
            then
                TEMP_DIR=$OPTARG
            fi
            ;;
        p)
            if [ "$PID" == "$EMPTY_STRING" ]
            then
                PID=$OPTARG
            fi
            ;;

    esac

done


# Log file for update process
echo "" > $LOG_FILE

# Verify if the update setup is correctly initialized
verifySetup
result=$?
if [ $result -ne $SUCCESS ]
then
    printError "Invalid setup : INS_14"
    exit $result
fi

# Update brackets
updateBrackets
