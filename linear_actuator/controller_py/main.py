from controller import controller

def main(args):
    controller.start(args.port, args.baud)


# help messages for cli here.

# run main controller
if __name__ == "__main__":
    main(args)