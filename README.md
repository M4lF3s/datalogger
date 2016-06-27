# Datalogger Readme

This file contains installation instructions for the NodeJS Server of this Project.

### Prequisities

* Install NodeJS from [https://nodejs.org] (Version: 4.4.6 LTS)
* Install the Git Commandline Interface from [https://git-scm.com]
  (Make sure you choose "Use Git from the Windows Command Prompt" during the Git    installation so the PATH Variable will get set)

### Installation
* Open a Commandline Window and type:
  ```
  > git clone https://github.com/mfraas64/datalogger.git
  ```
* Within the Commandline change to the directory
  ```
  > cd datalogger\NodeServer
  ```

  Then execute the following commands:
  ```
  > npm install
  > npm install -g bower
  > npm install -g grunt-cli
  > bower install
  > grunt build
  ```

This should completely install **Git**, **NodeJS** and the Node Packages **Bower** and **Grunt** along with the Dependencies defined within the Project.


### Starting the Server
* Within the Commandline change to the directory
  ```
  > cd datalogger\NodeServer
  ```
* To Start the Server type
  ```
  > node app.js
  ```

  You should see something like this:

  ![Screenshot](https://github.com/mfraas64/datalogger/raw/master/NodeServer/screenshot.png "Screenshot")


  Now you just need to open a Browser and go to
  `http://localhost:1337`


### Demo
[![Demo Video](http://img.youtube.com/vi/6nHwBDlBYts/0.jpg)](http://www.youtube.com/watch?v=6nHwBDlBYts)
  (Klick on the Image to open the Youtube-Video)

