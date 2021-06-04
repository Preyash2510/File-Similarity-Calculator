Multithreading

Names : Preyash Patel
        Jaini Patel

We created test sets which included different number of files and subdirectories.

TestSet1  contained two (.txt) files and a directory containing 1 (.txt) file and a subdirectory. 
Subdirectory contained 2 (.txt) files.
TestSet1 contained a very small of data as we checked the accurancy and results by hand calculations.
Once they were proven right. 

We made another TestSet2 which contained 3 (.txt) files, two directories.
One directory contained 1 text file and another conatined a subdirectory and 2 text files.
Out of all text files. One text file was empty, 2 text files were same.
There results of empty text file will be 0.70107 and the same text files will be 0. 
We got the same results after testing this second testSet.

We made sure that the optional arguements worked properly by giving them different values and testing them
with different values.

We also calculated the total number of comparisons by the given formula for each test cases.
In TestSet1, we calculated the mean frequency, KLD and JSD by hand to ensure that program works correctly.

We determined that our program is correct one we got the satisfactory results from the various test cases.

How to Use :- 

    In order to compile the program in terminal:
        ~make compare
    
    To enter the optional arguements :
        ./compare -d[Num of directory Threads] -f[NUmber of file threads] -a[Number of analysis threads] -s[suffix filename] [lists of files and directories.]

Once it complies successfully, it displayes the results as :
JSD value - files compared with its relative paths

//Displayes the details as :
Total files : //Total number of files provided
Total comparisons : //Total number of comparisons amde for each files 
# of Directory threads //Directory threads provided
# of File threads //File threads provided
# of Analysis threads //Amalysis threads provided
# of File suffix // Default is .txt if not provided.
