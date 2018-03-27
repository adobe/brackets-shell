#!/bin/sh

EMPTY_STRING=""

APP_NAME=$EMPTY_STRING
APP_DIR=$EMPTY_STRING
LOG_FILE=$EMPTY_STRING
MOUNT_POINT=$EMPTY_STRING
TEMP_DIR=$EMPTY_STRING

#BEGIN return value codes
SUCCESS=0
PS_FAILED=1
COPY_FAILED=3
OPTIONS_MISSING=7
MOVE_FAILED=8
CODESIGN_FAILED=9
DELETE_FAILED=10
#END return value codes

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
        printError "Brackets app dir is missing"
        return $OPTIONS_MISSING
    fi

    if [ "$APP_NAME" == "$EMPTY_STRING" ]
    then
        printError "Brackets app is missing"
        return $OPTIONS_MISSING
    fi
    
    if [ "$LOG_FILE" == "$EMPTY_STRING" ]
    then
        printError "Installer log file is missing"
        return $OPTIONS_MISSING
    fi
    
    if [ "$MOUNT_POINT" == "$EMPTY_STRING" ]
    then
        printError "DMG mount point is missing"
        return $OPTIONS_MISSING
    fi
    
    if [ "$TEMP_DIR" == "$EMPTY_STRING" ]
    then
        printError "Temp directory is missing"
        return $OPTIONS_MISSING
    fi
  
    return $SUCCESS
}

# Updates the brackets app
function updateBrackets(){
        
        local abortInstallation=0
        local returnValue=$SUCCESS
        

        if [ -d $APP_DIR ]
        then
            rm -rf "$TEMP_DIR/$APP_NAME"
            printInfo "Moving the existing Brackets.app to temp dir updateTemp..."
            mv "$APP_DIR/$APP_NAME" "$TEMP_DIR/$APP_NAME"
            
            exitStatus=$?
            if [ $exitStatus -ne 0 ]
            then
                printError "Unable to move the existing Brackets app to temp directory updateTemp. mv retured $exitStatus"
                abortInstallation=1
                returnValue=$MOVE_FAILED
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
                printError "Unable to copy Brackets.app. cp retured $exitStatus"
                abortInstallation=1
                returnValue=$COPY_FAILED
            else
                printInfo "Brackets.app is copied."
                
                printInfo "Verifying code signature of the new app."
                codesign -vvvv "$APP_DIR/$APP_NAME" > /dev/null 2>&1
                exitStatus=$?

                if [ $exitStatus -ne 0 ]
                then
                    printError "Unable to verify the downloaded $APP_DIR/$APP_NAME. codesign retured $exitStatus"
                    abortInstallation=1

                    printInfo "Deleting the new app copied."
                    rm -f $APP_DIR/$APP_NAME
                    exitStatus=$?
                    if [ $exitStatus -ne 0 ]
                    then
                        printError "Unable to delete $APP_DIR/$APP_NAME. rm retured $exitStatus"
                        returnValue=$DELETE_FAILED
                    else
                        printInfo "Moving the old brackets back from AppData"
                        mv "$TEMP_DIR/$APP_NAME" "$APP_DIR/$APP_NAME"
                        exitStatus=$?
                        if [ $exitStatus -ne 0 ]
                        then
                            printError "Unable to move the old brackets back from AppData. mv retured $exitStatus"
                        else
                            returnValue=$MOVE_FAILED
                        fi
                    fi
                    returnValue=$CODESIGN_FAILED
                else

                    printInfo "Code signature verified successfully."
                    printInfo "Marking Brackets as a trusted application..."
                    xattr -d -r com.apple.quarantine "$APP_DIR/$APP_NAME"
                    exitStatus=$?
                    if [ $exitStatus -ne 0 ]
                    then
                        printWarning "Unable to mark Brackets.app as trusted application. You may have to do it manually. xattr returnd $exitStatus. Please check the error log for details."
                    else
                        printInfo "Brackets.app is marked as trusted application."
                    
                    fi
                fi
            fi
        fi

        printInfo "Unmounting the DMG ..."
        hdiutil detach "$MOUNT_POINT" "-force" >> $LOG_FILE
        #AutoUpdate : TODO : Add the check for command success or failure here, and log accordingly
        printInfo "Unmounted the DMG and attempying to open Brackets ..."
        
        open -a "$APP_DIR/$APP_NAME"
        #AutoUpdate : TODO : Add the check for command success or failure here, and log accordingly
        printInfo "Opened Brackets"
        
        return $returnValue

}

# Parse the command line arguments
# -a - App name for currently installed Brackets
# -b - Absolute path to the currently installed Brackets App Directory
# -l - Absolute path to the updation log file path
# -m - Mount Point for the mounted disk image for latest Brackets version
# -t - Absolute path to the update temp directory in AppData
# -p - PID of the currently running brackets process    #AutoUpdate : TODO

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
    esac

done


# AutoUpdate : TODO: If $LOG_FILE exists, make sure that we have write permissions
echo "" > $LOG_FILE # To overwrite old content/create the file

verifySetup
result=$?
if [ $result -ne $SUCCESS ]
then
    printError "Invalid setup"
    exit $result
fi

updateBrackets
