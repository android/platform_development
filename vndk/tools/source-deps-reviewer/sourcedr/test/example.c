int main() {
    printf("This is a simple testing filen");
    "This line with dlopen shouldn't be found"
    'This line with dlopen shouldn\'t be found'
    /*
     * This dlopen shouldn't be found
     */
    dlopen("dlopen");
}
