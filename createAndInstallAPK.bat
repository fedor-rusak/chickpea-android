set DIRNAME=%~dp0

set CLASSPATH=%DIRNAME%gradle\lib\gradle-launcher-2.5.jar

SET BUILDNAME=installAllDebug

"%JAVA_HOME%\bin\java.exe" -classpath "%CLASSPATH%" org.gradle.launcher.GradleMain -g %DIRNAME%.gradle %BUILDNAME%