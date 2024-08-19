D='white'
import tkinter as B,tkinter.font as E
def F():
    try:
        with open('\\\\.\\pipe\\CommunicationPipe','wb')as A:A.write(C.get('1.0',B.END).encode('utf-8'))
    except FileNotFoundError:pass
A=B.Tk()
A.title('cumming executor')
A.geometry('600x400')
G=E.Font(family='Consolas',size=12)
C=B.Text(A,wrap=B.WORD,font=G,bg='#1e1e1e',fg='#d4d4d4',insertbackground=D)
C.grid(row=0,column=0,columnspan=2,sticky='nsew',padx=10,pady=10)
H=B.Button(A,text='runnnnn',command=F,bg='#007acc',fg=D)
H.grid(row=1,column=0,sticky='ew',padx=10,pady=10)
A.grid_rowconfigure(0,weight=1)
A.grid_columnconfigure([0,1],weight=1)
A.mainloop()
