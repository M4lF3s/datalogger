# Datalogger Readme

This file contains installation instructions for the NodeJS Server of this Project.

### Prequisities

* Install NodeJS from [https://nodejs.org] (Version: 4.4.6 LTS)
* Install the Git Commandline Interface from [https://git-scm.com]
  (Make sure you choose "Use Git from the Windows Command Prompt" during the Git    installation so the PATH Variable will get set)

### Installation
* Open a Commandline Window and type: 
  ```sh
  > git clone https://github.com/mfraas64/datalogger.git
  ```
* Within the Commandline change to the directory
  ```sh
  > cd datalogger\NodeServer
  ```

  Then execute the following commands:
  ```sh
  > npm install
  > npm install -g bower
  > npm install -g grunt-cli
  > bower install
  > grunt build
  ```

This should completely install **Git**, **NodeJS** and the Node Packages **Bower** and **Grunt** along with the Dependencies defined within the Project.


### Starting the Server
* Within the Commandline change to the directory
  ```sh
  > cd datalogger\NodeServer
  ```
* To Start the Server type
  ```sh
  > node app.js
  ```
  
  You should see something like this:
  
  ![alt text](https://github.com/mfraas64/datalogger/raw/master/NodeServer/screenshot.png "Screenshot")
