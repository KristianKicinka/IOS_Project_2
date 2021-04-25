#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>

#define BASE 10


// ERROR NUMBERS
typedef enum {
    PARAM_ERROR,
    FILE_ERROR,
    MEM_ERROR,
    SEM_ERROR,
    PROC_ERROR

}error_type;

// SANTA OUTPUT NUMBERS
typedef enum {
    SANTA_SLEEP,
    SANTA_HELPING,
    SANTA_CLOSING,
    SANTA_CHRISTMAS

}santa_texts;

// ELF OUTPUT NUMBERS
typedef enum {
    ELF_START,
    ELF_NEED_HELP,
    ELF_GET_HELP,
    ELF_HOLIDAY
}elf_texts;

// REINDEER OUTPUT NUMBERS
typedef enum {
    REINDEER_RST,
    REINDEER_HOME,
    REINDEER_GET
}reindeer_texts;

// Output file
FILE *out_file;

// Shared memory declaration
int *workshop_elf_counter;
int *active_reindeer_counter;
bool *workshop_state;
int *task_counter;
int *remaining_elves;

// Semaphores declaration
sem_t *santa_semaphore = NULL;
sem_t *reindeer_semaphore = NULL;
sem_t *elf_semaphore = NULL;
sem_t *elf_help_semaphore = NULL;
sem_t *christmas_semaphore = NULL;
sem_t *writing_semaphore = NULL;
sem_t *memory_semaphore = NULL;

// Program parameters structure
typedef struct prog_params{
    int elfs_count;
    int reindeers_count;
    int max_working_time;
    int max_holiday_time;
}program_parameters_t;

void init_program_parameters(program_parameters_t *program_parameters);
int max_duration(int duration);
void error_message(error_type error);
void initialize_semaphores();
void initialize_memory();
void uninitialize_semaphores();
void uninitialize_memory();
void santa_output_text(santa_texts text);
void elf_output_text(elf_texts text, int elf_id);
void reindeer_output_text(reindeer_texts text, int reindeer_id);
void santa_process(program_parameters_t *program_parameters);
void elf_process(int id ,program_parameters_t *program_parameters);
void reindeer_process(int id, program_parameters_t *program_parameters);

bool prepare_values(int argc, char *argv[], program_parameters_t *program_parameters);

int main( int argc, char *argv[] ) {
    program_parameters_t program_parameters;

    init_program_parameters(&program_parameters);
    bool prepare_values_error = prepare_values(argc, argv, &program_parameters);

    if(prepare_values_error)
        error_message(PARAM_ERROR);

    if((out_file = fopen("proj2.out","w")) == NULL)
        error_message(FILE_ERROR);
    
    initialize_semaphores();
    initialize_memory();

    (*workshop_elf_counter) = 0;
    (*active_reindeer_counter) = 0;
    (*workshop_state) = true; // false = closed ; true = open
    (*task_counter) = 0;
    
    // Creating needed processes
    int sum = program_parameters.elfs_count + program_parameters.reindeers_count;
    for (int id = 0; id < sum +1 ; id++){
        if(id < program_parameters.elfs_count +1){

            switch (fork()){
            case 0 :
                if(id == 0)
                    santa_process(&program_parameters);
                else
                elf_process(id,&program_parameters);
            break;
            case -1 : 
                error_message(PROC_ERROR);
                break;
            default :
                break;
            }
        }else{

            switch (fork()){
            case 0 :
                reindeer_process(id - program_parameters.elfs_count , &program_parameters);
                break;
            case -1 : 
                error_message(PROC_ERROR);
                break;
            default :
                break;
            }
        }
    }
    
    // Waiting for all processes
    for (int id = 0; id < sum + 1; id++){
        wait(NULL);
    }

    uninitialize_memory();
    uninitialize_semaphores();
    
    fclose(out_file);
    exit(0);
    return 0;
}

void initialize_semaphores(){
    bool error = false;

    if((santa_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED)
        error = true;
    if((reindeer_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED)
        error = true;
    if((writing_semaphore= mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED)
        error = true;
    if((memory_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED)
        error = true;
    if((elf_help_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED)
        error = true;
    if((elf_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED)
        error = true;
    if((christmas_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED)
        error = true;
        
    

    if((sem_init(santa_semaphore,1,0)) == -1)
        error = true;
    if((sem_init(reindeer_semaphore,1,0)) == -1)
        error = true;
    if((sem_init(elf_semaphore,1,0)) == -1)
        error = true;
    if((sem_init(writing_semaphore,1,1)) == -1)
        error = true;
    if((sem_init(memory_semaphore,1,1)) == -1)
        error = true;
    if((sem_init(elf_help_semaphore,1,0)) == -1)
        error = true;
    if((sem_init(christmas_semaphore,1,0)) == -1)
        error = true;
    
    if(error == true){
        uninitialize_memory();
        uninitialize_semaphores();
        error_message(SEM_ERROR);
    }    
}

void uninitialize_semaphores(){
    bool error = false;
    if((sem_destroy(santa_semaphore)) == -1)
        error = true;
    if((sem_destroy(reindeer_semaphore)) == -1)
        error = true;
    if((sem_destroy(elf_semaphore)) == -1)
        error = true;
    if((sem_destroy(writing_semaphore)) == -1)
        error = true;
    if((sem_destroy(memory_semaphore)) == -1)
        error = true;
    if((sem_destroy(elf_help_semaphore)) == -1)
        error = true;
    if((sem_destroy(christmas_semaphore)) == -1)
        error = true;
    
    munmap(santa_semaphore,sizeof(sem_t));
    munmap(reindeer_semaphore,sizeof(sem_t));
    munmap(elf_semaphore,sizeof(sem_t));
    munmap(writing_semaphore,sizeof(sem_t));
    munmap(memory_semaphore,sizeof(sem_t));
    munmap(elf_help_semaphore,sizeof(sem_t));
    munmap(christmas_semaphore,sizeof(sem_t));
    
    if(error == true){
        uninitialize_memory();
        uninitialize_semaphores();
        error_message(SEM_ERROR);
    }
}

void initialize_memory(){
    bool error = false;

    if ((workshop_elf_counter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
        error = true;
    if ((active_reindeer_counter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
        error = true;
    if ((workshop_state = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
        error = true;
    if ((task_counter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
        error = true;
    if ((remaining_elves = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED)
        error = true;

    if(error == true){
        uninitialize_memory();
        uninitialize_semaphores();
        error_message(MEM_ERROR);
    }

}

void uninitialize_memory(){

   munmap(workshop_elf_counter,sizeof(int));
   munmap(active_reindeer_counter,sizeof(int));
   munmap(workshop_state,sizeof(bool));
   munmap(task_counter,sizeof(int));
   munmap(remaining_elves,sizeof(int));

}

void santa_output_text(santa_texts text){
  
    sem_wait(writing_semaphore);
        *(task_counter)+=1;
        switch (text){
            case SANTA_SLEEP:
                fprintf(out_file,"%d: Santa: going to sleep\n", *(task_counter));
                fflush(NULL);
                break;
            case SANTA_HELPING:
                fprintf(out_file,"%d: Santa: helping elves\n", *(task_counter));
                fflush(NULL);
                break;
            case SANTA_CLOSING:
                fprintf(out_file,"%d: Santa: closing workshop\n", *(task_counter));
                fflush(NULL);
                break;
            case SANTA_CHRISTMAS:
                fprintf(out_file,"%d: Santa: Christmas started\n", *(task_counter));
                fflush(NULL);
                break;
        }
    sem_post(writing_semaphore);
    
}

void elf_output_text(elf_texts text, int elf_id){
    sem_wait(writing_semaphore);
        *(task_counter)+=1;
        switch (text){
            case ELF_START:
                fprintf(out_file,"%d: Elf %d: started\n", *(task_counter), elf_id);
                fflush(NULL);
                break;
            case ELF_NEED_HELP:
                fprintf(out_file,"%d: Elf %d: need help\n", *(task_counter), elf_id);
                fflush(NULL);
                break;
            case ELF_GET_HELP:
                fprintf(out_file,"%d: Elf %d: get help\n", *(task_counter),elf_id);
                fflush(NULL);
                break;
            case ELF_HOLIDAY:
                fprintf(out_file,"%d: Elf %d: taking holidays\n", *(task_counter),elf_id);
                fflush(NULL);
                break;
        }
    sem_post(writing_semaphore);
}

void reindeer_output_text(reindeer_texts text, int reindeer_id){
     sem_wait(writing_semaphore);
        *(task_counter)+=1;
        switch (text){
            case REINDEER_RST:
                fprintf(out_file,"%d: RD %d: rstarted\n", *(task_counter), reindeer_id);
                fflush(NULL);
                break;
            case REINDEER_HOME:
                fprintf(out_file,"%d: RD %d: return home\n", *(task_counter), reindeer_id);
                fflush(NULL);
                break;
            case REINDEER_GET:
                fprintf(out_file,"%d: RD %d: get hitched\n", *(task_counter), reindeer_id);
                fflush(NULL);
                break;
        }
    sem_post(writing_semaphore);
}

void santa_process(program_parameters_t *program_parameters){
    
    while (true){
        santa_output_text(SANTA_SLEEP);
        sem_wait(santa_semaphore);

        sem_wait(memory_semaphore);
        if((*active_reindeer_counter) == program_parameters->reindeers_count){
            (*workshop_state) = false;
            santa_output_text(SANTA_CLOSING);
            for (int i = 0; i < (*workshop_elf_counter); i++)
                sem_post(elf_help_semaphore);

            sem_post(memory_semaphore);
            break;

        }else if( (*workshop_elf_counter) == 3 && (*workshop_state) == true){
            santa_output_text(SANTA_HELPING);
            (*remaining_elves) = 3;
            for (int i = 0; i < 3; i++)
                sem_post(elf_help_semaphore);
            
            sem_post(memory_semaphore);
            sem_wait(elf_semaphore);
        }else{
            sem_post(memory_semaphore);
        }
        
    }

    for (int i = 0; i < program_parameters->reindeers_count; i++)
        sem_post(reindeer_semaphore);
    
    sem_wait(christmas_semaphore);
    santa_output_text(SANTA_CHRISTMAS);
    exit(0);
}

void elf_process(int id ,program_parameters_t *program_parameters){
    
    elf_output_text(ELF_START,id);

    while (true){

        usleep(max_duration(program_parameters->max_working_time));
        
        elf_output_text(ELF_NEED_HELP,id);

        sem_wait(memory_semaphore);
        if((*workshop_state) == false){
            sem_post(memory_semaphore);
            break;
        }

        (*workshop_elf_counter)+=1;

        if((*workshop_elf_counter) == 3 && (*workshop_state) == true )
            sem_post(santa_semaphore);
    
        sem_post(memory_semaphore);

        sem_wait(elf_help_semaphore);

        if((*workshop_state) == false){
            elf_output_text(ELF_HOLIDAY,id);
            exit(0);
        }
        elf_output_text(ELF_GET_HELP,id);

        (*remaining_elves)-=1;

        if ((*remaining_elves) == 0)
            sem_post(elf_semaphore);
        
        sem_wait(memory_semaphore);
        (*workshop_elf_counter)-=1;

        sem_post(memory_semaphore);
    }

    elf_output_text(ELF_HOLIDAY,id);
    exit(0);
}

void reindeer_process(int id, program_parameters_t *program_parameters){

    reindeer_output_text(REINDEER_RST,id);
    usleep(max_duration(program_parameters->max_holiday_time/2));

    reindeer_output_text(REINDEER_HOME,id);
    sem_wait(memory_semaphore);
    (*active_reindeer_counter)++;

    if((*active_reindeer_counter) == program_parameters->reindeers_count)
        sem_post(santa_semaphore);

    sem_post(memory_semaphore);
    sem_wait(reindeer_semaphore);

    reindeer_output_text(REINDEER_GET,id);
    (*active_reindeer_counter)--;
    if((*active_reindeer_counter) == 0)
        sem_post(christmas_semaphore);
    
    exit(0);
}


void init_program_parameters(program_parameters_t *program_parameters){
    program_parameters->elfs_count = 0;
    program_parameters->reindeers_count = 0;
    program_parameters->max_working_time = 0;
    program_parameters->max_holiday_time = 0;
}

void error_message(error_type error){
    switch (error){
        case PARAM_ERROR :
            fprintf(stderr, "Parameters loading error !!\n");
            exit(1);
            break;
        case FILE_ERROR : 
            fprintf(stderr, "Error with opening proj2.out file !!\n");
            exit(1);
            break;
        case PROC_ERROR : 
            fprintf(stderr, "Create process error !!\n");
            exit(1);
            break;
        default :
            fprintf(stderr, "Unexpected error !!\n");
            exit(1);
            break;
    }
}

int max_duration(int duration){
    if(duration != 0){
        srand(time(NULL));
        return (rand() % duration);
    }
    return 0;
}

bool prepare_values(int argc, char *argv[], program_parameters_t *program_parameters){
    int err_count = 0;
    char *tmp;
    if(argc < 5){
        return true;
    }
    int param_01 = strtol(argv[1],&tmp,BASE);
    if(*tmp =='\0' && param_01 > 0 && param_01 < 1000 ){
        program_parameters->elfs_count = param_01;
        err_count ++;
    }
    int param_02 = strtol(argv[2],&tmp,BASE);
    if (*tmp =='\0' && param_02 > 0 && param_02 < 20) {
        program_parameters->reindeers_count = param_02;
        err_count ++;
    }
    int param_03 = strtol(argv[3],&tmp,BASE);
    if (*tmp =='\0' && param_03 >= 0 && param_03 <= 1000){
        program_parameters->max_working_time = param_03;
        err_count ++;
    }
    int param_04 = strtol(argv[4],&tmp,BASE);    
    if(*tmp =='\0' && param_04 >= 0 && param_04 <= 1000){
        program_parameters->max_holiday_time = param_04;
        err_count ++;
    }
    if (err_count != 4)
        return true;
    
   return false;
}

